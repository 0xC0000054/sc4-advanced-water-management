////////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-advanced-water-management, a DLL Plugin for
// SimCity 4 that improves interoperability with advanced water management.
//
// Copyright (c) 2024 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////////

#include "cSC4ViewInputControlQueryHooks.h"
#include "Logger.h"
#include "SC4VersionDetection.h"
#include "GlobalPlumbingSim.h"
#include "WaterBuildingPropertyUtil.h"
#include "cIGZWin.h"
#include "cISC4Occupant.h"
#include "SC4Percentage.h"
#include <cstdint>
#include <Windows.h>
#include "wil/result.h"

namespace
{
	void InstallJumpTableHook(uintptr_t targetAddress, uintptr_t newValue)
	{
		// Allow the executable memory to be written to.
		DWORD oldProtect = 0;
		THROW_IF_WIN32_BOOL_FALSE(VirtualProtect(
			reinterpret_cast<LPVOID>(targetAddress),
			sizeof(uintptr_t),
			PAGE_EXECUTE_READWRITE,
			&oldProtect));

		// Patch the memory at the specified address.
		*reinterpret_cast<uintptr_t*>(targetAddress) = newValue;
	}
}

// Both SetPerformanceMeterValue and SetOrGetWaterBuildingEfficiency are
// non-virtual member functions of the cSC4ViewInputControlQuery class.

typedef bool(__cdecl* pfn_SetPerformanceMeterValue)(cIGZWin*, uint32_t, float);

static pfn_SetPerformanceMeterValue SetPerformanceMeterValue = nullptr;

static bool __cdecl Hook_SetOrGetWaterBuildingEfficiency(
	cIGZWin* pWin,
	uint32_t controlID,
	bool getValue,
	cISC4Occupant* pOccupant)
{
	if (!getValue)
	{
		return true;
	}

	SC4Percentage efficiency{};
	uint32_t currentCapacity = 0;
	uint32_t maxCapacity = 0;
	bool shutDownByPollution = false;

	pPlumbingSim->GetWaterProducerInfo(
		pOccupant,
		&currentCapacity,
		&maxCapacity,
		&efficiency,
		&shutDownByPollution,
		nullptr);

	float efficiencyPercentage = efficiency.percentage * 100.0f;

	if (currentCapacity == 0 && maxCapacity > 0 && !shutDownByPollution)
	{
		if (WaterBuildingPropertyUtil::IsNonAquiferWaterSource(pOccupant))
		{
			// If a water producer that uses fresh or salt water is not
			// producing any output, this indicates that the user placed
			// the producer too far from an appropriate water source.
			// We provide a visual indication of this by setting the
			// efficiency value to 0.

			efficiencyPercentage = 0.0f;
		}
	}

	bool result = SetPerformanceMeterValue(pWin, controlID, efficiencyPercentage);
	return result;
}

void cSC4ViewInputControlQueryHooks::Install()
{
	uintptr_t Hook_SetOrGetWaterBuildingEfficiency_Offset = 0;
	uintptr_t SetPerformanceMeterValue_Offset = 0;

	const uint16_t gameVersion = SC4VersionDetection::GetInstance().GetGameVersion();

	switch (gameVersion)
	{
	case 641:
		Hook_SetOrGetWaterBuildingEfficiency_Offset = 0xA92570;
		SetPerformanceMeterValue_Offset = 0x4C3CA0;
		break;
	}

	Logger& logger = Logger::GetInstance();

	if (Hook_SetOrGetWaterBuildingEfficiency_Offset != 0
		&& SetPerformanceMeterValue_Offset != 0)
	{
		try
		{
			InstallJumpTableHook(
				Hook_SetOrGetWaterBuildingEfficiency_Offset,
				reinterpret_cast<uintptr_t>(&Hook_SetOrGetWaterBuildingEfficiency));
			SetPerformanceMeterValue = reinterpret_cast<pfn_SetPerformanceMeterValue>(SetPerformanceMeterValue_Offset);

			logger.WriteLine(LogLevel::Info, "Installed the water building query UI hook.");
		}
		catch (const wil::ResultException& e)
		{
			logger.WriteLineFormatted(
				LogLevel::Error,
				"Failed to install the water building query UI hook.\n%s",
				e.what());
		}
	}
	else
	{
		logger.WriteLineFormatted(
			LogLevel::Error,
			"Failed to install the water building query UI hook. "
			"Requires game version 641, found game version %d.",
			gameVersion);
	}
}
