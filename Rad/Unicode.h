#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>
#include <string_view>

// https://learn.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
#define CP_UTF16_LE     1200u
#define CP_UTF16_BE     1201u
#define CP_UTF32_LE     12000u
#define CP_UTF32_BE     12001u
#define CP_UNKNOWN      UINT(-1)

inline bool IsWide16(_In_ const UINT cp) { return cp == CP_UTF16_LE || cp == CP_UTF16_BE; }
inline bool IsWide32(_In_ const UINT cp) { return cp == CP_UTF32_LE || cp == CP_UTF32_BE; }

inline bool IsLittleEndian(_In_ const UINT cp) { return cp == CP_UTF16_LE || cp == CP_UTF32_LE; }
inline bool IsBigEndian(_In_ const UINT cp) { return cp == CP_UTF16_BE || cp == CP_UTF32_BE; }

template <class Alloc>
inline void MultiByteToWideChar(_In_ const UINT CodePage, _In_ const DWORD dwFlags, _In_ const std::string_view line, _Out_ std::basic_string<wchar_t, std::char_traits<wchar_t>, Alloc>& wline)
{
    wline.resize(line.size());
    const int s = ::MultiByteToWideChar(CodePage, dwFlags, line.data(), static_cast<int>(line.size()), wline.data(), static_cast<int>(wline.size()));
    wline.resize(s);
}

template <class Alloc>
inline void WideCharToMultiByte(_In_ const UINT CodePage, _In_ const DWORD dwFlags, _In_ const std::wstring_view wline, _Out_ std::basic_string<char, std::char_traits<char>, Alloc>& line, _In_opt_ LPCCH lpDefaultChar = nullptr, _Out_opt_ LPBOOL lpUsedDefaultChar = nullptr)
{
    line.resize(wline.size() * 2);
    const int s = ::WideCharToMultiByte(CodePage, dwFlags, wline.data(), static_cast<int>(wline.size()), line.data(), static_cast<int>(line.size()), lpDefaultChar, lpUsedDefaultChar);
    line.resize(s);
}

inline UINT GuessCodePage(_In_reads_bytes_(iSize) CONST VOID* lpv, _In_ const int iSize, _In_ const UINT defcp)
{
    INT Test = IS_TEXT_UNICODE_UNICODE_MASK | IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_UTF8;
    if (IsTextUnicode(lpv, iSize, &Test))
    {
        if (Test & IS_TEXT_UNICODE_UNICODE_MASK)
            return CP_UTF16_LE;
        else if (Test & IS_TEXT_UNICODE_REVERSE_MASK)
            return CP_UTF16_BE;
        else if (Test & IS_TEXT_UNICODE_UTF8)
            return CP_UTF8;
        else
            return CP_UTF16_LE;
    }
    else
        return defcp;
}
