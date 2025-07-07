#pragma once

#include <cstddef>
#include <utility>

template <size_t sz>
void byte_swap(std::byte* data);

template <>
inline void byte_swap<1>(std::byte* data)
{
    // do nothing
}

template <>
inline void byte_swap<2>(std::byte* data)
{
    std::swap(data[0], data[1]);
}

template <size_t sz>
void byte_swap_span(dyn_span<std::byte> data)
{
    _ASSERTE(data.size() % sz == 0);
    for (dyn_span<std::byte>::iterator ita = data.begin(); ita < std::prev(data.end()); ita += sz)
        byte_swap<sz>(&*ita);
}

template <>
inline void byte_swap_span<1>(dyn_span<std::byte> data)
{
    // do nothing
}
