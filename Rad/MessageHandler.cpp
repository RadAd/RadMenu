#include "MessageHandler.h"
#include <algorithm>

LRESULT MessageHandler::ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
{
    Message m = { uMsg, wParam, lParam, false };
    Message* const pMsg = std::exchange(m_msg, &m);

    LRESULT ret = 0;
    try
    {
        ret = HandleMessage(uMsg, wParam, lParam);
    }
    catch (...)
    {
#if _UNICODE
        _RPTFW0(_CRT_ERROR, TEXT("Unhandled exception"));
#else
        _RPTF0(_CRT_ERROR, TEXT("Unhandled exception"));
#endif
    }

    _ASSERTE(m_msg == &m);
    std::exchange(m_msg, pMsg);

    if (m_delete && m_msg == nullptr)
        delete this;

    bHandled = m.m_bHandled;
    return ret;
}


LRESULT MessageChain::ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
{
    Message m = { hWnd, uMsg, wParam, lParam, false };
    Message* const pMsg = std::exchange(m_msg, &m);

    LRESULT ret = 0;
    try
    {
        ret = HandleMessage(uMsg, wParam, lParam);
    }
    catch (...)
    {
#if _UNICODE
        _RPTFW0(_CRT_ERROR, TEXT("Unhandled exception"));
#else
        _RPTF0(_CRT_ERROR, TEXT("Unhandled exception"));
#endif
    }

    _ASSERTE(m_msg == &m);
    std::exchange(m_msg, pMsg);

    bHandled = m.m_bHandled;
    return ret;
}
