#pragma once
#include "RZBaseWinProc.h"
#include "cIGZWinMgr.h"
#include "GZServPtrs.h"

class SC4GenericDialogWinProc : public RZBaseWinProc
{
public:

	SC4GenericDialogWinProc();

	bool DoWinProcMessage(cIGZWin* pWin, cGZMessage& pMsg);

private:

	cIGZWinMgrPtr pWinMgr;
};

