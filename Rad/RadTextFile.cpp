#include "RadTextFile.h"

#include "ByteOrderMark.h"
#include "byte_swap.h"

// RadITextFile

bool RadITextFile::ReadLine(std::string& line, const UINT outCodePage)
{
    _ASSERTE(m_cp != CP_UNKNOWN);
    _ASSERTE(!IsWide16(outCodePage));
    if (IsWide16(m_cp))
    {
        thread_local std::wstring wline;
        const bool r = ReadLineInternal(wline, IsBigEndian(m_cp));
        if (r)
            WideCharToMultiByte(outCodePage, 0, wline, line);
        else
            line.clear();
        return r;
    }
    else
    {
        const bool r = ReadLineInternal(line);
        if (r && m_cp != outCodePage && outCodePage != CP_UNKNOWN)
        {
            thread_local std::wstring wline;
            MultiByteToWideChar(m_cp, 0, line, wline);
            WideCharToMultiByte(outCodePage, 0, wline, line);
        }
        return r;
    }
}

bool RadITextFile::ReadLine(std::wstring& wline, const UINT outCodePage)
{
    _ASSERTE(m_cp != CP_UNKNOWN);
    _ASSERTE(IsWide16(outCodePage));
    if (IsWide16(m_cp))
        return ReadLineInternal(wline, IsLittleEndian(m_cp) != IsLittleEndian(outCodePage));
    else
    {
        thread_local std::string line;
        const bool r = ReadLineInternal(line);
        if (r)
        {
            MultiByteToWideChar(m_cp, 0, line, wline);
            if (IsBigEndian(outCodePage))
                byte_swap_span<sizeof(std::wstring::value_type)>(to_dyn_span(wline).reinterpret<std::byte>());
        }
        else
            wline.clear();
        return r;
    }
}

void RadITextFile::DetermineEncoding(UINT defcp)
{
    if (m_buffer.size() < 3)
        FillBuffer();
    const ByteOrderMark enc = ::DetermineByteOrderMark(m_buffer);
    switch (enc)
    {
    case ByteOrderMark::UTF8:       m_cp = CP_UTF8; break;
    case ByteOrderMark::UTF16_LE:   m_cp = CP_UTF16_LE; break;
    case ByteOrderMark::UTF16_BE:   m_cp = CP_UTF16_BE; break;
    case ByteOrderMark::UTF32_LE:   m_cp = CP_UTF32_LE; break;
    case ByteOrderMark::UTF32_BE:   m_cp = CP_UTF32_BE; break;
    default:                        m_cp = GuessCodePage(m_buffer.data(), static_cast<int>(m_buffer.size()), defcp); break;
    }
}

template <class T>
bool RadITextFile::ReadLineInternal(T& line, bool swap)
{
    auto it = std::find(m_buffer.begin(), m_buffer.end(), std::byte('\n'));
    if (it != m_buffer.end())
        return extract(line, it + (IsLittleEndian(m_cp) ? 2 : 1), swap), true;

    while (true)
    {
        const bool more = FillBuffer();
        if (m_buffer.empty())
            return line.clear(), false;

        it = std::find(m_buffer.begin(), m_buffer.end(), std::byte('\n'));
        if (it != m_buffer.end())
            return extract(line, it + (IsLittleEndian(m_cp) ? 2 : 1), swap), true;

        if (!more)
            return extract(line, m_buffer.end(), swap), true;
    }
}

bool RadITextFile::FillBuffer()
{
    const size_t size = m_buffer.size();
    m_buffer.resize(4096);
    const size_t read = m_file.Read(to_dyn_span(m_buffer).subspan(size));
    m_buffer.resize(size + read);
    return read > 0;
}

template <class T>
void RadITextFile::extract(T& line, std::vector<std::byte>::iterator it, bool swap)
{
    _ASSERTE(!m_buffer.empty());
    const auto dist = std::distance(m_buffer.begin(), it);
    _ASSERTE(dist % sizeof(T::value_type) == 0);
    if (swap)
        byte_swap_span<sizeof(T::value_type)>(to_dyn_span(m_buffer).first(dist));
    line.assign(reinterpret_cast<T::pointer>(m_buffer.data()), reinterpret_cast<T::pointer>(m_buffer.data() + dist));
    m_buffer.erase(m_buffer.begin(), it);
}

// RadOTextFile

bool RadOTextFile::Write(std::string_view line, const UINT inCodePage)
{
    _ASSERTE(m_cp != CP_UNKNOWN);
    _ASSERTE(!IsWide16(inCodePage));
    if (IsWide16(m_cp))
    {
        std::wstring wline;
        MultiByteToWideChar(m_cp, 0, line, wline);
        return m_file.WriteAll(to_dyn_span(wline).reinterpret<const std::byte>());
    }
    else if (m_cp != inCodePage && inCodePage != CP_UNKNOWN)
    {
        std::string templine;
        std::wstring wline;
        MultiByteToWideChar(inCodePage, 0, line, wline);
        WideCharToMultiByte(m_cp, 0, wline, templine);
        return m_file.WriteAll(to_dyn_span(templine).reinterpret<const std::byte>());
    }
    else
    {
        return m_file.WriteAll(to_const_dyn_span(line).reinterpret<const std::byte>());
    }
}

bool RadOTextFile::Write(std::wstring_view wline, const UINT inCodePage)
{
    _ASSERTE(m_cp != CP_UNKNOWN);
    _ASSERTE(IsWide16(inCodePage));
    if (IsWide16(m_cp))
    {
        if (m_cp != inCodePage)
        {
            auto sp = to_const_dyn_span(wline).reinterpret<const std::byte>();
            std::vector<std::byte> temp(sp.begin(), sp.end());
            byte_swap_span<sizeof(std::wstring::value_type)>(to_dyn_span(temp));
            return m_file.WriteAll(to_dyn_span(temp));
        }
        else
            return m_file.WriteAll(to_const_dyn_span(wline).reinterpret<const std::byte>());
    }
    else
    {
        std::string templine;
        WideCharToMultiByte(m_cp, 0, wline, templine);
        return m_file.WriteAll(to_dyn_span(templine).reinterpret<const std::byte>());
    }
}

void RadOTextFile::WriteBom()
{
    const UINT cp = m_cp != CP_ACP ? m_cp : GetACP();
    switch (cp)
    {
    case CP_UTF8:       m_file.Write(GetByteOrderMark(ByteOrderMark::UTF8)); break;
    case CP_UTF16_LE:   m_file.Write(GetByteOrderMark(ByteOrderMark::UTF16_LE)); break;
    case CP_UTF16_BE:   m_file.Write(GetByteOrderMark(ByteOrderMark::UTF16_BE)); break;
    case CP_UTF32_LE:   m_file.Write(GetByteOrderMark(ByteOrderMark::UTF32_LE)); break;
    case CP_UTF32_BE:   m_file.Write(GetByteOrderMark(ByteOrderMark::UTF32_BE)); break;
    }
}
