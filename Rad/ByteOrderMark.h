#pragma once

#include <cstddef>
#include <type_traits>
#include <vector>
#include <crtdbg.h>
#include "span.h"

constexpr std::byte operator ""_b(const unsigned long long x)
{
    _ASSERTE(x <= 0xFF);
    return std::byte(x);
}

template <class E>
std::underlying_type_t<E> to_underlying(const E e)
{
    return static_cast<std::underlying_type_t<E>>(e);
}

enum class ByteOrderMark
{
    Unknown,
    UTF8,
    UTF16_BE,
    UTF16_LE,
    UTF32_BE,
    UTF32_LE,
};

static const std::byte bom_utf8[] = { 0xef_b, 0xbb_b, 0xbf_b };
static const std::byte bom_utf16_be[] = { 0xfe_b, 0xff_b };
static const std::byte bom_utf16_le[] = { 0xff_b, 0xfe_b };
static const std::byte bom_utf32_be[] = { 0x00_b, 0x00_b, 0xfe_b, 0xff_b };
static const std::byte bom_utf32_le[] = { 0xff_b, 0xfe_b, 0x00_b, 0x00_b };

static const dyn_span<const std::byte> bom[] = {
    bom_utf8,
    bom_utf16_be,
    bom_utf16_le,
    bom_utf32_be,
    bom_utf32_le,
};

inline dyn_span<const std::byte> GetByteOrderMark(const ByteOrderMark enc)
{
    return bom[to_underlying(enc) - to_underlying(ByteOrderMark::UTF8)];
}

inline ByteOrderMark DetermineByteOrderMark(std::vector<std::byte>& buffer)
{
    _ASSERTE(buffer.size() >= 3);

    for (const ByteOrderMark enc : { ByteOrderMark::UTF8, ByteOrderMark::UTF16_LE, ByteOrderMark::UTF16_BE, ByteOrderMark::UTF32_LE, ByteOrderMark::UTF32_BE })
    {
        const dyn_span<const std::byte> encbom = GetByteOrderMark(enc);
        if (to_dyn_span(buffer).first(encbom.size()) == encbom)
        {
            buffer.erase(buffer.begin(), buffer.begin() + encbom.size());
            return enc;
        }
    }

    return ByteOrderMark::Unknown;
}
