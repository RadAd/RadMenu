#pragma once

#include "RadFile.h"

#include <vector>
#include <string>
#include <string_view>

#include "Unicode.h"

class RadITextFile
{
public:
    static RadITextFile StdIn(UINT defcp = CP_ACP)
    {
        return RadITextFile(GetStdHandle(STD_INPUT_HANDLE), defcp);
    }

    RadITextFile()
    {
    }

    RadITextFile(_In_ HANDLE hFile, _In_ UINT defcp)
        : m_file(hFile, false)
    {
        if (Valid())
            DetermineEncoding(defcp);
    }

    RadITextFile(_In_ RadIFile File, _In_ UINT defcp)
        : m_file(std::move(File))
    {
        if (Valid())
            DetermineEncoding(defcp);
    }

    RadITextFile(_In_ LPCTSTR lpFileName, UINT defcp)
        : m_file(lpFileName)
    {
        if (Valid())
            DetermineEncoding(defcp);
    }

    bool Open(_In_ LPCTSTR lpFileName, UINT defcp)
    {
        m_file.Open(lpFileName);
        if (Valid())
            DetermineEncoding(defcp);
    }

    bool Valid() const { return m_file.Valid(); }

    UINT GetCodePage() const { return m_cp; }

    bool ReadLine(std::string& line, const UINT outCodePage);
    bool ReadLine(std::wstring& wline, const UINT outCodePage);

private:
    void DetermineEncoding(UINT defcp);
    template <class T> bool ReadLineInternal(T& line, bool swap = false);
    bool FillBuffer();
    template <class T> void extract(T& line, std::vector<std::byte>::iterator it, bool swap);

    RadIFile m_file;
    std::vector<std::byte> m_buffer;
    UINT m_cp = CP_UNKNOWN;
};

class RadOTextFile
{
public:
    static RadOTextFile StdOut(UINT defcp = CP_ACP)
    {
        const HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        return RadOTextFile(h, defcp);
    }

    static RadOTextFile StdErr(UINT defcp = CP_ACP)
    {
        const HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
        return RadOTextFile(h, defcp);
    }

    RadOTextFile()
    {
    }

    RadOTextFile(_In_ const HANDLE hFile, _In_ const UINT cp, _In_ const bool bom)
        : m_file(hFile, false), m_cp(cp)
    {
        if (bom)
            WriteBom();
    }

    RadOTextFile(_In_ const HANDLE hFile, _In_ const UINT cp)
        : m_file(hFile, false), m_cp(cp)
    {
        if (GetFileType(hFile) != FILE_TYPE_CHAR)
            WriteBom();
    }

    RadOTextFile(_In_ LPCTSTR lpFileName, _In_ const UINT cp, _In_ const bool bom)
        : m_file(lpFileName), m_cp(cp)
    {
        if (bom)
            WriteBom();
    }

    bool Open(_In_ LPCTSTR lpFileName, _In_ const UINT cp, _In_ const bool bom)
    {
        m_file.Open(lpFileName);
        m_cp = cp;
        if (bom)
            WriteBom();
    }

    bool Valid() const { return m_file.Valid(); }

    UINT GetCodePage() const { return m_cp; }

    bool Write(std::string_view line, const UINT inCodePage);
    bool Write(std::wstring_view wline, const UINT inCodePage);

    bool WriteLine(std::string_view line, const UINT inCodePage)
    {
        return Write(line, inCodePage) && Write("\r\n", inCodePage);
    }

    bool WriteLine(std::wstring_view wline, const UINT inCodePage)
    {
        return Write(wline, inCodePage) && Write(L"\r\n", inCodePage);
    }

private:
    void WriteBom();

    RadOFile m_file;
    UINT m_cp = CP_UNKNOWN;
};
