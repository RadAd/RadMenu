#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>

#ifdef _UNICODE
#define tstringstream wstringstream
#else
#define tstringstream stringstream
#endif

inline LPCTSTR strBegin(LPCTSTR s) { return s; }
inline LPCTSTR strEnd(LPCTSTR s) { return s + lstrlen(s); }

inline bool StrFindI(LPCTSTR s1, LPCTSTR s2)
{
    auto it = std::search(
        strBegin(s1), strEnd(s1),
        strBegin(s2), strEnd(s2),
        [](TCHAR ch1, TCHAR ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    return (it != strEnd(s1));
}

template <class CharType, class Traits, class Allocator>
std::basic_istream<CharType, Traits>& getlinequotes(
    std::basic_istream<CharType, Traits>& in_stream,
    std::basic_string<CharType, Traits, Allocator>& str,
    const CharType delimiter)
{
#if 0
    return std::getline(in_stream, str, delimiter);
#else
    str.clear();
    CharType c;
    if (in_stream.get(c))
    {
        if (c != delimiter)
        {
            bool inquote = c == CharType('"');
            str.push_back(c);
            while (in_stream.get(c) and (inquote or c != delimiter))
            {
                str.push_back(c);
                if (c == CharType('"')) inquote = !inquote;
            }
        }
        if (!in_stream.bad() and in_stream.eof())
            in_stream.clear(std::ios_base::eofbit);
    }
    return in_stream;
#endif
}

inline void unquote(std::tstring& str)
{
    if (str.length() >= 2)
    {
        if (str.back() == TEXT('"')) str.erase(str.end() - 1);
        if (str.front() == TEXT('"')) str.erase(str.begin());
    }
}

inline std::vector<std::tstring> split(const std::tstring& str, TCHAR delim)
{
    std::vector<std::tstring> split;
    std::tstringstream ss(str);
    std::tstring sstr;
    while (getlinequotes(ss, sstr, delim))
        //if (!sstr.empty())
        split.push_back(sstr);
    return split;
}

inline std::vector<std::tstring> split_unquote(const std::tstring& str, TCHAR delim)
{
    std::vector<std::tstring> a = split(str, delim);
    for (auto& s : a)
        unquote(s);
    return a;
}
