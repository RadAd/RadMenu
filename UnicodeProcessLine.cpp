#include "UnicodeProcessLine.h"

#include <assert.h>
#include <vector>
#include <string>

void MultiByteToWideChar(_In_ UINT CodePage, _In_ DWORD dwFlags, const std::string_view line, std::wstring& wline)
{
    wline.resize(line.size());
    const int s = ::MultiByteToWideChar(CodePage, dwFlags, line.data(), static_cast<int>(line.size()), wline.data(), static_cast<int>(wline.size()));
    wline.resize(s);
}

enum class Encoding
{
    Unknown,
    UTF8,
    UTF16_BE,
    UTF16_LE,
    CODE_PAGE,
};

void ProcessBuffer(std::vector<BYTE>& buffer, const Encoding enc, _In_ UINT CodePage, std::wstring& wline, ProcessLine ppl, void* data, const bool all)
{
    switch (enc)
    {
    case Encoding::UTF16_BE:
    case Encoding::UTF16_LE:
    {
        std::wstring_view str(reinterpret_cast<const wchar_t*>(buffer.data()), buffer.size() / sizeof(wchar_t));
        size_t b = 0;
        size_t e = str.find(L'\n', b);
        while (e != std::string_view::npos)
        {
            const std::wstring_view line(str.substr(b, e - b + 1));
            ppl(line, data);
            b = e + 1;
            e = str.find('\n', b);
        }
        if (all)
        {
            const std::wstring_view line(str.substr(b));
            ppl(line, data);
            b = line.size();
        }
        buffer.erase(buffer.begin(), buffer.begin() + b * sizeof(wchar_t));
        break;
    }
    case Encoding::UTF8:
        CodePage = CP_UTF8;
        [[fallthrough]];
    case Encoding::CODE_PAGE:
    {
        std::string_view str(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        size_t b = 0;
        size_t e = str.find('\n', b);
        while (e != std::string_view::npos)
        {
            const std::string_view line(str.substr(b, e - b + 1));
            MultiByteToWideChar(CodePage, 0, line, wline);
            ppl(wline, data);
            b = e + 1;
            e = str.find('\n', b);
        }
        if (all)
        {
            const std::string_view line(str.substr(b));
            MultiByteToWideChar(CodePage, 0, line, wline);
            ppl(wline, data);
            b = line.size();
        }
        buffer.erase(buffer.begin(), buffer.begin() + b);
        break;
    }
    }
}

bool UnicodeProcessLine(const HANDLE hFile, _In_ UINT CodePage, ProcessLine ppl, void* data)
{
    Encoding enc = Encoding::Unknown;
    std::vector<BYTE> buffer;
    std::wstring wline;
    size_t swapoffset = 0;
    while (true)
    {
        const size_t offset = buffer.size();
        buffer.resize(4096);

        DWORD read = 0;
        if (!ReadFile(hFile, buffer.data() + offset, static_cast<DWORD>(buffer.size() - offset), &read, nullptr))
        {
            DWORD e = GetLastError();
            if (e != ERROR_BROKEN_PIPE)
                return false;
        }
        buffer.resize(read + offset);
        if (read == 0)
            break;

        if (enc == Encoding::Unknown)
        {
            if (buffer.size() >= 3 && buffer[0] == 0xef && buffer[1] == 0xbb && buffer[0xbf])
            {
                buffer.erase(buffer.begin(), buffer.begin() + 3);
                enc = Encoding::UTF8;
            }
            else if (buffer.size() >= 2 && buffer[0] == 0xff && buffer[1] == 0xfe)
            {
                buffer.erase(buffer.begin(), buffer.begin() + 2);
                enc = Encoding::UTF16_LE;
            }
            else if (buffer.size() >= 2 && buffer[0] == 0xfe && buffer[1] == 0xff)
            {
                buffer.erase(buffer.begin(), buffer.begin() + 2);
                enc = Encoding::UTF16_BE;
            }
            else if (IsTextUnicode(buffer.data(), static_cast<int>(buffer.size()), nullptr))
                enc = Encoding::UTF16_LE;
            else
                enc = Encoding::CODE_PAGE;
        }

        if (enc == Encoding::UTF16_BE)
        {
            for (std::vector<BYTE>::iterator ita = buffer.begin() + offset - swapoffset; ita < std::prev(buffer.end()); ita += 2)
            {
                std::vector<BYTE>::iterator itb = std::next(ita);
                std::swap(*ita, *itb);
            }
            swapoffset = buffer.size() % 2;
        }

        ProcessBuffer(buffer, enc, CodePage, wline, ppl, data, false);
    }
    ProcessBuffer(buffer, enc, CodePage, wline, ppl, data, true);
    assert(buffer.empty());
    assert(swapoffset == 0);
    return false;
}
