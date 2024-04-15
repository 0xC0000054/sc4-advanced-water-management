#include "RZBaseWinProc.h"

RZBaseWinProc::RZBaseWinProc() : refCount(0)
{
}

bool RZBaseWinProc::QueryInterface(uint32_t riid, void** ppvObj)
{
    if (riid == GZIID_cIGZWinProc)
    {
        *ppvObj = static_cast<cIGZWinProc*>(this);
        AddRef();

        return true;
    }
    else if (riid == GZIID_cIGZUnknown)
    {
        *ppvObj = static_cast<cIGZUnknown*>(this);
        AddRef();

        return true;
    }

    return false;
}

uint32_t RZBaseWinProc::AddRef()
{
    return ++refCount;
}

uint32_t RZBaseWinProc::Release()
{
    if (refCount > 0)
    {
        --refCount;
    }

    return refCount;
}

bool RZBaseWinProc::DoWinProcMessage(cIGZWin* pWin, cGZMessage& pMsg)
{
    return false;
}

bool RZBaseWinProc::DoWinMsg(cIGZWin* pWin, uint32_t dwMessageID, uint32_t data1, uint32_t data2, uint32_t data3)
{
    return false;
}
