#include "SC4NotificationDialog.h"
#include "cGZMessage.h"
#include "cGZPersistResourceKey.h"
#include "cIGZWinText.h"
#include "SC4UI.h"
#include "StringResourceManager.h"
#include "cISC4City.h"
#include "cISC424HourClock.h"
#include "cISC4Simulator.h"

namespace
{
	void CenterDialogOnParent(const cIGZWin* parent, cIGZWin* child)
	{
		const int32_t parentWidth = parent->GetW();
		const int32_t parentHeight = parent->GetH();

		const int32_t childWidth = child->GetW();
		const int32_t childHeight = child->GetW();

		int newX = (parentWidth - childWidth) / 2;
		int newY = (parentHeight - childHeight) / 3;

		child->GZWinMoveTo(newX, newY);
	}

	void SetStaticLabelText(cIGZWin* pDialog, uint32_t controlID, const StringResourceKey& textKey)
	{
		cIGZWinText* pWinText = nullptr;

		if (pDialog->GetChildAsRecursive(
			controlID,
			GZIID_cIGZWinText,
			reinterpret_cast<void**>(&pWinText)))
		{
			cIGZString* pText = nullptr;

			if (StringResourceManager::GetLocalizedString(textKey, &pText))
			{
				pWinText->SetCaption(*pText);

				pText->Release();
			}

			pWinText->Release();
		}
	}
}

SC4NotificationDialog::SC4NotificationDialog(
	StringResourceKey const& messageKey,
	StringResourceKey const& captionKey)
	: messageKey(messageKey),
	  captionKey(captionKey),
	  pScriptDialog(nullptr),
	  pDialog(nullptr)
{
}

void SC4NotificationDialog::ShowDialog()
{
	PauseGame();

	if (CreateDialog())
	{
		pScriptDialog->PullToFront();
		pWinMgr->AddWindowToValidList(pScriptDialog);

		pWinMgr->DoModalWin(pScriptDialog);

		pWinMgr->RemoveWindowFromValidList(pScriptDialog);
		pWinMgr->CleanUpWindowReferences(pScriptDialog);

		DestroyDialog();
	}

	ResumeGame();
}

bool SC4NotificationDialog::DoWinProcMessage(cIGZWin* pWin, cGZMessage& pMsg)
{
	// These constants were taken from SC4's built-in *WinProc classes.
	// The message constants are probably unique to GZCOM, but the design of
	// the windows messaging system appears to be based on the system used by
	// Microsoft Windows.
	//
	// A dwMesageType value of 3 is a 'Command' message (similar to WM_COMMAND).
	// For the 'Command' type the other data fields appear to be:
	// dwData1: The command ID.
	// dwData2: The control ID.

	constexpr uint32_t kGZWin_MessageType_Command = 3;

	constexpr uint32_t kGZWin_Command_ButtonClicked = 0x287259F6;

	if (pMsg.dwMessageType == kGZWin_MessageType_Command
		&& pMsg.dwData1 == kGZWin_Command_ButtonClicked)
	{
		constexpr uint32_t GZWin_RequestClose = 0xf;

		// This is not safe for 64-bit. But SC4 passes the window pointer
		// to GZWinMsgPost as an integer, so we do the same.

		const uint32_t windowAddress = reinterpret_cast<uint32_t>(pWin);

		pWinMgr->GZWinMsgPost(
			GZWin_RequestClose,
			windowAddress,
			pMsg.dwData2);

		return true;
	}


	return false;
}

bool SC4NotificationDialog::CreateDialog()
{
	constexpr uint32_t kGZWin_WinSC4App = 0x6104489A;

	cIGZWin* pMainWindow = pWinMgr->GetMainWindow();

	cIGZWin* pSC4AppWin = pMainWindow->GetChildWindowFromID(kGZWin_WinSC4App);

	if (!pSC4AppWin)
	{
		return false;
	}

	cGZPersistResourceKey key(0, 0x96A006B0, 0xCA8CBF0F);
	constexpr uint32_t kWindowCLSID = 0xAA8DEF97;

	pScriptDialog = SC4UI::CreateWindowFromScript(key, pSC4AppWin, kWindowCLSID);

	if (!pScriptDialog)
	{
		return false;
	}

	if (!pScriptDialog->QueryInterface(GZIID_cIGZWinGen, reinterpret_cast<void**>(&pDialog)))
	{
		pScriptDialog->Release();

		return false;
	}

	pDialog->SetWinProc(this);

	CenterDialogOnParent(pSC4AppWin, pScriptDialog);

	SetStaticLabelText(pScriptDialog, 0xCA8CC4B6, captionKey);
	SetStaticLabelText(pScriptDialog, 0x4A8CC4A9, messageKey);

	return true;
}

void SC4NotificationDialog::DestroyDialog()
{
	if (pDialog)
	{
		pDialog->SetWinProc(nullptr);

		pDialog->Release();
		pDialog = nullptr;
	}

	if (pScriptDialog)
	{
		pScriptDialog->Release();
		pScriptDialog = nullptr;
	}
}

void SC4NotificationDialog::PauseGame()
{
	cISC4City* pCity = pSC4App->GetCity();

	if (pCity)
	{
		cISC424HourClock* pClock = pCity->Get24HourClock();
		cISC4Simulator* pSim = pCity->GetSimulator();

		if (pClock && pSim)
		{
			pClock->HiddenPause(true);
			pSim->HiddenPause();
		}
	}
}

void SC4NotificationDialog::ResumeGame()
{
	cISC4City* pCity = pSC4App->GetCity();

	if (pCity)
	{
		cISC424HourClock* pClock = pCity->Get24HourClock();
		cISC4Simulator* pSim = pCity->GetSimulator();

		if (pClock && pSim)
		{
			pClock->HiddenPause(false);
			pSim->HiddenResume();
		}
	}
}
