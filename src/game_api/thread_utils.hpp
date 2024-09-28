#pragma once

#include <Windows.h> // for ResumeThread, SuspendThread, HANDLE
#include <cstddef>   // for size_t
#include <cstdint>   // for int64_t

HANDLE get_main_thread();
size_t heap_base();
size_t local_heap_base();

// Used for objects that are allocated with the game's custom allocator
template <typename T>
class OnHeapPointer
{
    int64_t ptr_;
    friend struct std::hash<OnHeapPointer<T>>;

  public:
    explicit OnHeapPointer(size_t ptr)
        : ptr_(ptr)
    {
    }

    static OnHeapPointer from_raw_ptr(T* raw_ptr)
    {
        return OnHeapPointer(reinterpret_cast<size_t>(raw_ptr) - local_heap_base());
    }

    T* decode()
    {
        return reinterpret_cast<T*>(ptr_ + heap_base());
    }

    T* decode_local()
    {
        auto lhb = local_heap_base();
        if (lhb == 0)
            return nullptr;

        return reinterpret_cast<T*>(ptr_ + lhb);
    }

    T* operator->()
    {
        return decode();
    }
    bool operator==(const OnHeapPointer& other) const
    {
        return ptr_ == other.ptr_;
    }
};

template<typename T>
struct std::hash<OnHeapPointer<T>>
{
    size_t operator()(const OnHeapPointer<T>& k) const
    {
        return std::hash<int64_t>()(k.ptr_);
    }
};

struct CriticalSection
{
    HANDLE thread;

    CriticalSection()
    {
        thread = get_main_thread();
        SuspendThread(thread);
    }

    ~CriticalSection()
    {
        ResumeThread(thread);
    }
};
