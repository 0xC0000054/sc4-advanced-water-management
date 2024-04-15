#include "SC4GenericDialogWinProc.h"
#include "cGZMessage.h"
#include "cIGZWinMgr.h"

SC4GenericDialogWinProc::SC4GenericDialogWinProc()
{
}

bool SC4GenericDialogWinProc::DoWinProcMessage(cIGZWin* pWin, cGZMessage& pMsg)
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

		return 1;
	}

	return 0;
}
