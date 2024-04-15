#pragma once

#include "StringResourceKey.h"
#include "RZBaseWinProc.h"
#include "cIGZWin.h"
#include "cIGZWinGen.h"
#include "cISC4App.h"
#include "cIGZWinMgr.h"
#include "GZServPtrs.h"
#include <memory>

// SC4's standard notification dialog with an OK button.
// TGI 0, 0x96A006B0, 0xCA8CBF0F
class SC4NotificationDialog : protected RZBaseWinProc
{
public:

	SC4NotificationDialog(StringResourceKey const& messageKey, StringResourceKey const& captionKey);

	void ShowDialog();

private:

	bool DoWinProcMessage(cIGZWin* pWin, cGZMessage& pMsg) override;

	bool CreateDialog();

	void DestroyDialog();

	void PauseGame();

	void ResumeGame();

	StringResourceKey messageKey;
	StringResourceKey captionKey;
	cIGZWinMgrPtr pWinMgr;
	cISC4AppPtr pSC4App;
	cIGZWin* pScriptDialog;
	cIGZWinGen* pDialog;
};

