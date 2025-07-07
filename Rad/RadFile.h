#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <memory>

#include "span.h"

BOOL WINAPI NoCloseHandle(_In_ _Post_ptr_invalid_ HANDLE hObject);

class RadFile
{
public:
    explicit RadFile(HANDLE hFile = NULL, bool own = true)
        : m_hFile(hFile, own ? CloseHandle : NoCloseHandle)
    {
    }

    RadFile(RadFile&&) = default;

    RadFile& operator=(RadFile&&) = default;

    virtual ~RadFile()
    {
        Close();
    }

    operator HANDLE() const { return m_hFile.get(); }

    bool Valid() const { return m_hFile != NULL; }

    bool Open(
        _In_ LPCTSTR lpFileName,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition,
        _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile)
    {
        Close();
        const HANDLE hFile = CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        if (hFile == INVALID_HANDLE_VALUE)
            return false;
        m_hFile.reset(hFile);
        return true;
    }

    void Close()
    {
        m_hFile.reset();
    }

private:
    std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&CloseHandle)> m_hFile;
};

class RadIFile : public RadFile
{
public:
    using RadFile::RadFile;

    explicit RadIFile(_In_ LPCTSTR lpFileName)
    {
        if (lpFileName)
            Open(lpFileName);
    }

    bool Open(_In_ LPCTSTR lpFileName)
    {
        return Open(lpFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    DWORD Read(
        _Out_writes_bytes_(nNumberOfBytesToRead) __out_data_source(FILE) LPVOID lpBuffer,
        _In_ DWORD nNumberOfBytesToRead)
    {
        DWORD nNumberOfBytesRead = 0;
        const BOOL b = ReadFile(*this, lpBuffer, nNumberOfBytesToRead, &nNumberOfBytesRead, nullptr);
        if (b)
            return nNumberOfBytesRead;
        else
        {
            _ASSERTE(nNumberOfBytesRead == 0);
            return 0;
        }
    }

    size_t Read(dyn_span<std::byte> buffer) // TODO should use void
    {
        return Read(buffer.data(), static_cast<DWORD>(buffer.size()));
    }

private:
    using RadFile::Open;
};

class RadOFile : public RadFile
{
public:
    using RadFile::RadFile;

    explicit RadOFile(_In_ LPCTSTR lpFileName)
    {
        if (lpFileName)
            Open(lpFileName);
    }

    bool Open(_In_ LPCTSTR lpFileName)
    {
        return Open(lpFileName, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    DWORD Write(
        _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
        _In_ DWORD nNumberOfBytesToWrite)
    {
        DWORD pNumberOfBytesWritten = 0;
        const BOOL b = WriteFile(*this, lpBuffer, nNumberOfBytesToWrite, &pNumberOfBytesWritten, nullptr);
        if (b)
            return pNumberOfBytesWritten;
        else
        {
            _ASSERTE(pNumberOfBytesWritten == 0);
            return 0;
        }
    }

    size_t Write(dyn_span<const std::byte> buffer) // TODO should use void
    {
        return Write(buffer.data(), static_cast<DWORD>(buffer.size()));
    }

    bool WriteAll(dyn_span<const std::byte> buffer)
    {
        while (!buffer.empty())
        {
            const size_t written = Write(buffer);
            if (written == 0)
                return false;   // TODO Can this happen?
            buffer = buffer.subspan(written);
        }
        return true;
    }

private:
    using RadFile::Open;
};
