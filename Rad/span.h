#pragma once

//#include <initializer_list>
#include <utility>
#include <crtdbg.h>

// To be replaced by std::span in C++20
// TODO Create an iterator for begin/end which can validate when derefenced

template <class T>
class dyn_span
{
public:
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T* iterator;
    typedef const T* const_iterator;

    dyn_span()
        : m_begin(nullptr), m_end(nullptr)
    {
    }

    dyn_span(pointer begin, pointer end)
        : m_begin(begin), m_end(end)
    {
        _ASSERTE(m_end >= m_begin);
    }

#if 0
    dyn_span(const std::initializer_list<T>& il)
        : m_begin(il.begin()), m_end(il.end())
    {
    }
#endif

    template <size_t sz>
    dyn_span(T(&a)[sz])
        : m_begin(a), m_end(a + sz)
    {
    }

    operator dyn_span<const value_type>() const { return { m_begin, m_end }; }

    template <class U>
    dyn_span<U> reinterpret() const { return { reinterpret_cast<U*>(m_begin), reinterpret_cast<U*>(m_end) }; }

    pointer data() { return m_begin; }
    const_pointer data() const { return m_begin; }

    bool empty() const { return m_begin == m_end; }

    iterator begin() { return m_begin; }
    iterator end() { return m_end; }
    const_iterator begin() const { return m_begin; }
    const_iterator end() const { return m_end; }
    size_t size() const { return m_end - m_begin; }

    reference operator[](size_t i) { validate_index(i); return m_begin[i]; }
    const_reference operator[](size_t i) const { validate_index(i); return m_begin[i]; }

    dyn_span first(size_t count) const { return { m_begin, std::min(m_begin + count, m_end) }; }
    dyn_span last(size_t count) const { return { std::max(m_end - count, m_begin), m_end }; }
    dyn_span subspan(size_t offset) const { return { std::min(m_begin + offset, m_end), m_end }; }
    dyn_span subspan(size_t offset, size_t count) const { return { std::min(m_begin + offset, m_end), std::min(m_begin + offset + count, m_end) }; }

private:
    void validate_index(size_t i) const { _ASSERTE(i < size()); }
    pointer m_begin;
    pointer m_end;
};

template <class U, class T = typename U::value_type>
dyn_span<T> to_dyn_span(U& v)
{
    return { v.data(), v.data() + v.size() };
}

template <class U, class T = const typename U::value_type>
dyn_span<T> to_dyn_span(const U& v)
{
    return { v.data(), v.data() + v.size() };
}

template <class U, class T = typename U::value_type>
dyn_span<const T> to_const_dyn_span(U& v)
{
    return { v.data(), v.data() + v.size() };
}

template <class T>
bool operator==(dyn_span<const T> a, dyn_span<const T> b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i])
            return false;
    return true;
}

template <class T>
bool operator==(dyn_span<T> a, dyn_span<const T> b)
{
    return dyn_span<const T>(a) == b;
}

template <class T>
bool operator==(dyn_span<const T> a, dyn_span<T> b)
{
    return a == dyn_span<const T>(b);
}
