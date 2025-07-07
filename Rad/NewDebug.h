#pragma once
#include <crtdbg.h>

#ifdef _DEBUG
#define DEBUG_NEW_BLOCK(x) (x, __FILE__, __LINE__)
#else
#define DEBUG_NEW_BLOCK(x)
#endif

#define DEBUG_NEW DEBUG_NEW_BLOCK(_NORMAL_BLOCK)

#include <limits>
#include <new>

template <typename T, int type>
class DebugAllocator
{
public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    DebugAllocator() noexcept {}

    template <typename U>
    DebugAllocator(const DebugAllocator<U, type>&) noexcept {}

    pointer allocate(size_type n)
    {
        if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
            throw std::bad_alloc(); // Handle potential overflow
        }
        if constexpr (type == _IGNORE_BLOCK)
        {
            const int debug_flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
            _CrtSetDbgFlag(debug_flag & ~_CRTDBG_ALLOC_MEM_DF);
            pointer p = new T[n];
            _CrtSetDbgFlag(debug_flag);
            return p;
        }
        else
        {
            pointer p = new DEBUG_NEW_BLOCK(type) T[n];
            return p;
        }
    }

    void deallocate(pointer p, size_type n) noexcept
    {
        delete[] p;
    }

    template <typename U>
    bool operator==(const DebugAllocator<U, type>&) const noexcept
    {
        return true; // Simple allocator, no state to compare
    }

    template <typename U>
    bool operator!=(const DebugAllocator<U, type>& other) const noexcept
    {
        return !(*this == other);
    }

    // Rebind struct (required for C++98/03, good practice for modern C++)
    template <typename U>
    struct rebind
    {
        typedef DebugAllocator<U, type> other;
    };
};
