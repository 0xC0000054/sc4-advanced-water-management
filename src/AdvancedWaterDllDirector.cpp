////////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-advanced-water, a DLL Plugin for SimCity 4
// that improves interoperability with advanced water management.
//
// Copyright (c) 2024, 2025 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////////

#include "version.h"
#include "Logger.h"
#include "WaterBuildingOccupantFilter.h"
#include "cIBuildingQueryHookServer.h"
#include "cIBuildingQueryCustomToolTipHookTarget.h"
#include "cIGZCOM.h"
#include "cIGZFrameWork.h"
#include "cIGZApp.h"
#include "cIGZCheatCodeManager.h"
#include "cIGZLanguageManager.h"
#include "cGZPersistResourceKey.h"
#include "cIGZPersistResourceManager.h"
#include "cISC4AdviceSubject.h"
#include "cISC4AdvisorSystem.h"
#include "cISC4App.h"
#include "cISC4City.h"
#include "cISC4BuildingOccupant.h"
#include "cISC4Occupant.h"
#include "cISC4OccupantManager.h"
#include "cISC4PlumbingSimulator.h"
#include "cISCProperty.h"
#include "cISCPropertyHolder.h"
#include "cIGZVariant.h"
#include "cIGZWin.h"
#include "cIGZWinMgr.h"
#include "cIGZMessageServer2.h"
#include "cIGZMessageTarget.h"
#include "cIGZMessageTarget2.h"
#include "cIGZString.h"
#include "cRZMessage2COMDirector.h"
#include "cRZMessage2Standard.h"
#include "cRZBaseString.h"
#include "GZCLSIDDefs.h"
#include "GZServPtrs.h"
#include "SC4Percentage.h"
#include "StringResourceKey.h"
#include "StringResourceManager.h"
#include "SC4NotificationDialog.h"
#include "SC4VersionDetection.h"
#include "cSC4ViewInputControlQueryHooks.h"
#include "WaterBuildingPropertyUtil.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <Windows.h>
#include "wil/resource.h"
#include "wil/result.h"
#include "wil/win32_helpers.h"

static constexpr uint32_t kMessageCheatIssued = 0x230E27AC;
static constexpr uint32_t kSC4MessageSimNewMonth = 0x66956816;
static constexpr uint32_t kSC4MessagePostCityInit = 0x26D31EC1;
static constexpr uint32_t kSC4MessagePreCityShutdown = 0x26D31EC2;

static constexpr uint32_t kAdvancedWaterModDirectorID = 0xCFE4D72D;

static const uint32_t kPumpHasNoFreshWaterSourceEventID = 0xCB94C1DD;
static const uint32_t kOccupantGroup_Building_Water = 0x1401;

static constexpr uint32_t ScanPumpsCheatID = 0x8E9F3621;

static constexpr std::string_view PluginLogFileName = "AdvancedWaterManagement.log";

cISC4PlumbingSimulator* pPlumbingSim = nullptr;

namespace
{
	void ShowBrokenWaterBuildingDialog(uint32_t ltextMessageInstanceID, uint32_t ltextCaptionInstanceID)
	{
		constexpr uint32_t kLTEXTGroupID = 0x6A231EA4;

		SC4NotificationDialog::ShowDialog(
			StringResourceKey(kLTEXTGroupID, ltextMessageInstanceID),
			StringResourceKey(kLTEXTGroupID, ltextCaptionInstanceID));
	}

	static bool WaterBuildingIterator(cISC4Occupant* pOccupant, void* pData)
	{
		WaterBuildingPropertyUtil::WaterSource source = WaterBuildingPropertyUtil::WaterSource::Aquifer;

		if (WaterBuildingPropertyUtil::IsWaterProducer(pOccupant, source))
		{
			// We look for water producers that use fresh or salt water which
			// are not producing any output.
			// This can indicate that the user placed the producer too far from
			// an appropriate water source.

			if (source == WaterBuildingPropertyUtil::WaterSource::FreshWater
				|| source == WaterBuildingPropertyUtil::WaterSource::SaltWater)
			{
				uint32_t currentCapacity = pPlumbingSim->GetWaterProduction(pOccupant);

				if (currentCapacity == 0)
				{
					uint32_t messageInstanceID = 0;
					uint32_t captionInstanceID = 0;

					switch (source)
					{
					case WaterBuildingPropertyUtil::WaterSource::FreshWater:
						messageInstanceID = 0xC38E721E;
						captionInstanceID = 0x6242E7DF;
						break;
					case WaterBuildingPropertyUtil::WaterSource::SaltWater:
						messageInstanceID = 0x00C64A89;
						captionInstanceID = 0x503E14E3;
						break;
					case WaterBuildingPropertyUtil::WaterSource::Aquifer:
					default:
						break;
					}

					if (messageInstanceID != 0 && captionInstanceID != 0)
					{
						ShowBrokenWaterBuildingDialog(messageInstanceID, captionInstanceID);
						// Only show one message per month.
						return false;
					}
				}
			}
		}

		return true;
	}

	bool GetBuildingOccupantName(cISC4Occupant* occupant, cIGZString& destination)
	{
		bool result = false;

		if (occupant)
		{
			cRZAutoRefCount<cISC4BuildingOccupant> buildingOccupant;

			if (occupant->QueryInterface(GZIID_cISC4BuildingOccupant, buildingOccupant.AsPPVoid()))
			{
				result = buildingOccupant->GetName(destination);
			}
		}

		return result;
	}

	bool SetLocalizedStringValue(uint32_t ltextGroupID, uint32_t ltextInstanceID, cIGZString& destination)
	{
		bool result = false;

		StringResourceKey key(ltextGroupID, ltextInstanceID);

		cRZAutoRefCount<cIGZString> localizedString;

		if (StringResourceManager::GetLocalizedString(key, localizedString))
		{
			destination.Copy(*localizedString);
			result = true;
		}

		return result;
	}
}

class AdvancedWaterModDllDirector : public cRZMessage2COMDirector, cIBuildingQueryCustomToolTipHookTarget
{
public:

	AdvancedWaterModDllDirector()
	{
		std::filesystem::path dllFolderPath = GetDllFolderPath();

		std::filesystem::path logFilePath = dllFolderPath;
		logFilePath /= PluginLogFileName;

		Logger& logger = Logger::GetInstance();
		logger.Init(logFilePath, LogLevel::Error);
		logger.WriteLogFileHeader("AdvancedWaterManagement v" PLUGIN_VERSION_STR);
	}

	uint32_t GetDirectorID() const
	{
		return kAdvancedWaterModDirectorID;
	}

	bool QueryInterface(uint32_t riid, void** ppvObj)
	{
		if (riid == GZIID_cIBuildingQueryCustomToolTipHookTarget)
		{
			*ppvObj = static_cast<cIBuildingQueryCustomToolTipHookTarget*>(this);
			AddRef();

			return true;
		}

		return cRZMessage2COMDirector::QueryInterface(riid, ppvObj);
	}

	uint32_t AddRef()
	{
		return cRZMessage2COMDirector::AddRef();
	}

	uint32_t Release()
	{
		return cRZMessage2COMDirector::Release();
	}

	bool ProcessToolTip(
		cISC4Occupant* const occupant,
		bool debugQuery,
		cIGZString& title,
		cIGZString& text,
		uint32_t& backgroundImageIID,
		uint32_t& meterImageIID,
		float& meterPercentage)
	{
		constexpr uint32_t kUtilityToolTipBackgroundBitmapIID = 0x1441620A;
		constexpr uint32_t kWaterBuildingMeterBitmapIID = 0x14416219;

		bool result = false;

		if (!debugQuery && occupant && occupant->IsOccupantGroup(kOccupantGroup_Building_Water))
		{
			if (GetBuildingOccupantName(occupant, title))
			{
				SC4Percentage efficiency{};
				uint32_t currentCapacity = 0;
				uint32_t maxCapacity = 0;
				bool shutDownByPollution = false;

				pPlumbingSim->GetWaterProducerInfo(
					occupant,
					&currentCapacity,
					&maxCapacity,
					&efficiency,
					&shutDownByPollution,
					nullptr);

				// Both the background and meter image must be set for the meter to appear.
				backgroundImageIID = kUtilityToolTipBackgroundBitmapIID;
				meterImageIID = kWaterBuildingMeterBitmapIID;
				meterPercentage = efficiency.percentage;

				if (shutDownByPollution)
				{
					result = SetLocalizedStringValue(0xEA5524EB, 0x4A5FB66A, text);
				}
				else
				{
					if (currentCapacity == 0 && maxCapacity > 0)
					{
						if (WaterBuildingPropertyUtil::IsNonAquiferWaterSource(occupant))
						{
							// If a water producer that uses fresh or salt water is not
							// producing any output, this indicates that the user placed
							// the producer too far from an appropriate water source.
							// We provide a visual indication of this by setting the
							// efficiency value to 0.
							meterPercentage = 0.0f;
						}
					}
					result = true;
				}
			}
		}

		return result;
	}

	void ScanForNonWorkingWaterProducers()
	{
		cISC4AppPtr pSC4App;
		if (pSC4App)
		{
			cISC4City* pCity = pSC4App->GetCity();

			if (pCity)
			{
				cISC4OccupantManager* pOccupantManager = pCity->GetOccupantManager();

				if (pOccupantManager)
				{
					WaterBuildingOccupantFilter waterBuildingFilter;

					pOccupantManager->IterateOccupants(
						WaterBuildingIterator,
						nullptr,
						nullptr,
						nullptr,
						&waterBuildingFilter);
				}
			}
		}
	}

	void ProcessCheat(cIGZMessage2Standard* pStandardMsg)
	{
		uint32_t cheatID = static_cast<uint32_t>(pStandardMsg->GetData1());

		if (cheatID == ScanPumpsCheatID)
		{
			ScanForNonWorkingWaterProducers();
		}
	}

	void PostCityInit(cIGZMessage2Standard* pStandardMsg)
	{
		cISC4City* pCity = static_cast<cISC4City*>(pStandardMsg->GetVoid1());

		if (pCity)
		{
			pPlumbingSim = pCity->GetPlumbingSimulator();

			cIGZCOM* const pCOM = GZCOM();

			cRZAutoRefCount<cIBuildingQueryHookServer> hookServer;

			if (pCOM->GetClassObject(
				GZCLSID_cIBuildingQueryHookServer,
				GZIID_cIBuildingQueryHookServer,
				hookServer.AsPPVoid()))
			{
				hookServer->AddNotification(this);
			}
			else
			{
				Logger& logger = Logger::GetInstance();
				logger.WriteLine(LogLevel::Error, "Failed to load the cIBuildingQueryHookServer.");
			}
		}
	}

	void PreCityShutdown()
	{
		pPlumbingSim = nullptr;

		cIGZCOM* const pCOM = GZCOM();

		cRZAutoRefCount<cIBuildingQueryHookServer> hookServer;

		if (pCOM->GetClassObject(
			GZCLSID_cIBuildingQueryHookServer,
			GZIID_cIBuildingQueryHookServer,
			hookServer.AsPPVoid()))
		{
			hookServer->RemoveNotification(this);
		}
	}

	bool DoMessage(cIGZMessage2* pMessage)
	{
		cIGZMessage2Standard* pStandardMsg = static_cast<cIGZMessage2Standard*>(pMessage);
		uint32_t dwType = pMessage->GetType();

		switch (dwType)
		{
		case kMessageCheatIssued:
			ProcessCheat(pStandardMsg);
			break;
		case kSC4MessageSimNewMonth:
			ScanForNonWorkingWaterProducers();
			break;
		case kSC4MessagePostCityInit:
			PostCityInit(pStandardMsg);
			break;
		case kSC4MessagePreCityShutdown:
			PreCityShutdown();
			break;
		}

		return true;
	}

	bool PostAppInit()
	{
		Logger& logger = Logger::GetInstance();

		cISC4AppPtr pSC4App;
		if (pSC4App)
		{
			cIGZCheatCodeManager* pCheatMgr = pSC4App->GetCheatCodeManager();

			if (pCheatMgr)
			{
				pCheatMgr->AddNotification2(this, 0);
				pCheatMgr->RegisterCheatCode(ScanPumpsCheatID, cRZBaseString("ScanPumps"));
			}
		}

		cIGZMessageServer2Ptr pMsgServ;
		if (pMsgServ)
		{
			std::vector<uint32_t> requiredNotifications;
			requiredNotifications.push_back(kSC4MessageSimNewMonth);
			requiredNotifications.push_back(kSC4MessagePostCityInit);
			requiredNotifications.push_back(kSC4MessagePreCityShutdown);

			for (uint32_t messageID : requiredNotifications)
			{
				if (!pMsgServ->AddNotification(this, messageID))
				{
					logger.WriteLine(LogLevel::Error, "Failed to subscribe to the required notifications.");
					return false;
				}
			}
		}
		else
		{
			logger.WriteLine(LogLevel::Error, "Failed to subscribe to the required notifications.");
			return false;
		}

		return true;
	}

	bool OnStart(cIGZCOM* pCOM)
	{
		cSC4ViewInputControlQueryHooks::Install();

		cIGZFrameWork* const pFramework = RZGetFrameWork();

		if (pFramework->GetState() < cIGZFrameWork::kStatePreAppInit)
		{
			pFramework->AddHook(this);
		}
		else
		{
			PreAppInit();
		}
		return true;
	}

private:

	std::filesystem::path GetDllFolderPath()
	{
		wil::unique_cotaskmem_string modulePath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

		std::filesystem::path temp(modulePath.get());

		return temp.parent_path();
	}
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static AdvancedWaterModDllDirector sDirector;
	return &sDirector;
}