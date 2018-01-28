//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \brief Generic singleton adaptor for C++ classes.
 *
 *  Each adaptor uses the curiously recurring pattern to enforce
 *  a singleton pattern for the wrapped class. There is both a heap
 *  and stack singleton, the former of which uses dynamic memory
 *  to allocate the singleton, and the latter using an uninitalized
 *  buffer on the stack.
 *
 *  For `stack_singleton`, since it requires an incomplete type,
 *  both the type-size and type-alignment **must** be provided.
 *  This assertion is checked at compile-time in the destructor.
 *
 *  To avoid superfluous overhead for single-threaded conditions,
 *  each singleton comes in multi- and single-threaded variants.
 *  The multi-threaded variant uses atomics to ensure a mutex is
 *  only locked during initialization, with notable performance gains.
 *
 *  In debug builds, the singleton pattern asserts in the destructor if
 *  it is used outside of a singleton policy. For example:
 *
 *  \code
 *      struct my_struct: heap_singleton<my_struct>
 *      {};
 *
 *      int main()
 *      {
 *          my_struct s;                        // will lead to abort();
 *          mystruct& s1 = my_struct::get();    // fine
 *      }
 *
 *  \synopsis
 *      template <typename T, bool ThreadSafe>
 *      class heap_singleton
 *      {
 *      public:
 *          static constexpr bool thread_safe = ThreadSafe;
 *          using value_type = T;
 *
 *          template<typename ... Ts>
 *          static value_type& get(Ts... ts);
 *
 *      protected:
 *          heap_singleton() = default;
 *          heap_singleton(const heap_singleton&) = delete;
 *          heap_singleton& operator=(const heap_singleton&) = delete;
 *          ~heap_singleton();
 *      };
 *
 *      template <typename T, size_t Size, size_t Alignment, bool ThreadSafe>
 *      class stack_singleton
 *      {
 *      public:
 *          static constexpr bool thread_safe = ThreadSafe;
 *          using value_type = T;
 *          using storage_type = aligned_storage_t<Size, Alignment>;
 *
 *          template<typename... Ts>
 *          static value_type& get(Ts... ts);
 *
 *      protected:
 *          stack_singleton() = default;
 *          stack_singleton(const stack_singleton&) = delete;
 *          stack_singleton& operator=(const stack_singleton&) = delete;
 *          ~stack_singleton();
 *      };
 */

#pragma once

#include <pycpp/config.h>
#include <pycpp/adaptor/stack_pimpl.h>
#include <pycpp/stl/atomic.h>
#include <pycpp/stl/cassert.h>
#include <pycpp/stl/memory.h>
#include <pycpp/stl/mutex.h>

PYCPP_BEGIN_NAMESPACE

// OBJECTS
// -------

/**
 *  \brief Optionally thread-safe heap singleton pattern.
 *
 *  Use `dummy_mutex` to disable thread-safety. Since the object is
 *  only allocated a single time, the global allocator is sufficient.
 */
template <typename T, bool ThreadSafe = true>
class heap_singleton;

// Single-threaded
template <typename T>
class heap_singleton<T, false>
{
public:
    static constexpr bool thread_safe = false;
    using value_type = T;

    template<typename ... Ts>
    static
    value_type&
    get(
        Ts... ts
    )
    {
        if (data_ == nullptr) {
            data_ = new value_type(forward<Ts>(ts)...);
        }
        return *data_;
    }

protected:
    heap_singleton() = default;
    heap_singleton(const heap_singleton&) = delete;
    heap_singleton& operator=(const heap_singleton&) = delete;

    ~heap_singleton()
    {
        // use temporary to avoid recursion
        value_type* tmp = data_;
        data_ = nullptr;
        delete tmp;
#ifndef NDEBUG
        assert(tmp != nullptr && "Singleton used outside of pattern.");
#endif
    }

private:
    static value_type* data_;
};

template <typename T>
T*
heap_singleton<T, false>::data_ = nullptr;


// Multi-threaded
template <typename T>
class heap_singleton<T, true>
{
public:
    static constexpr bool thread_safe = true;
    using value_type = T;

    template<typename ... Ts>
    static
    value_type&
    get(
        Ts... ts
    )
    {
        T* p = data_.load(memory_order_acquire);
        if (p == nullptr) {
            lock_guard<mutex> lock(mu_);
            p = data_.load(memory_order_relaxed);
            if (p == nullptr) {
                p = new T(forward<Ts>(ts)...);
                data_.store(p, memory_order_release);
            }
        }
        return *p;
    }

protected:
    heap_singleton() = default;
    heap_singleton(const heap_singleton&) = delete;
    heap_singleton& operator=(const heap_singleton&) = delete;

    ~heap_singleton()
    {
        // use temporary to avoid recursion
        value_type* tmp = data_.load(memory_order_acquire);
        data_.store(nullptr, memory_order_release);
        delete tmp;
#ifndef NDEBUG
        assert(tmp != nullptr && "Singleton used outside of pattern.");
#endif
    }

private:
    static atomic<value_type*> data_;
    static mutex mu_;
};

template <typename T>
atomic<T*>
heap_singleton<T, true>::data_ = ATOMIC_VAR_INIT(nullptr);

template <typename T>
mutex
heap_singleton<T, true>::mu_;

/**
 *  \brief Optionally thread-safe stack singleton pattern.
 *
 *  Use `dummy_mutex` to disable thread-safety. Since the object is
 *  only allocated a single time, the global allocator is sufficient.
 *
 *  The stack singleton **must** known the type size prior to instantiation,
 *  since the CRTP works with incomplete types. For safety reasons,
 *  this assertion is checked in the destructor, leading to a compiler
 *  error if the wrong size or alignment is used.
 */
template <
    typename T,
    size_t Size,
    size_t Alignment = alignof(max_align_t),
    bool ThreadSafe = true
>
class stack_singleton;

// Single-threaded
template <
    typename T,
    size_t Size,
    size_t Alignment
>
class stack_singleton<T, Size, Alignment, false>
{
public:
    static constexpr bool thread_safe = false;
    using value_type = T;
    using storage_type = aligned_storage_t<Size, Alignment>;

    template<typename... Ts>
    static
    value_type&
    get(
        Ts... ts
    )
    {
        value_type& r = reinterpret_cast<value_type&>(data_);
        if (!initialized_) {
            new (&r) value_type(forward<Ts>(ts)...);
            initialized_ = true;
        }
        return r;
    }

protected:
    stack_singleton() = default;
    stack_singleton(const stack_singleton&) = delete;
    stack_singleton& operator=(const stack_singleton&) = delete;

    ~stack_singleton()
    {
        // use a temporary to avoid recursion
        pimp_detail::storage_asserter<T, Size, Alignment> {};
        value_type& r = reinterpret_cast<value_type&>(data_);
        bool tmp = initialized_;
        initialized_ = false;
        if (tmp) {
            r.~T();
        }
#ifndef NDEBUG
        assert(tmp && "Singleton used outside of pattern.");
#endif
    }

private:
    static storage_type data_;
    static bool initialized_;
};

template <typename T, size_t Size, size_t Alignment>
aligned_storage_t<Size, Alignment>
stack_singleton<T, Size, Alignment, false>::data_;

template <typename T, size_t Size, size_t Alignment>
bool
stack_singleton<T, Size, Alignment, false>::initialized_ = false;

// Multi-threaded
template <
    typename T,
    size_t Size,
    size_t Alignment
>
class stack_singleton<T, Size, Alignment, true>
{
public:
    static constexpr bool thread_safe = true;
    using value_type = T;
    using storage_type = aligned_storage_t<Size, Alignment>;

    template<typename... Ts>
    static
    value_type&
    get(
        Ts... ts
    )
    {
        value_type& r = reinterpret_cast<value_type&>(data_);
        if (!initialized_.load()) {
            lock_guard<mutex> lock(mu_);
            if (!initialized_.load()) {
                new (&r) value_type(forward<Ts>(ts)...);
                initialized_.store(true, memory_order_release);
            }
        }
        return r;
    }

protected:
    stack_singleton() = default;
    stack_singleton(const stack_singleton&) = delete;
    stack_singleton& operator=(const stack_singleton&) = delete;

    ~stack_singleton()
    {
        pimp_detail::storage_asserter<T, Size, Alignment> {};
        value_type& r = reinterpret_cast<value_type&>(data_);
        bool tmp = initialized_.load(memory_order_acquire);
        initialized_.store(false, memory_order_release);
        if (tmp) {
            r.~T();
        }
#ifndef NDEBUG
        assert(tmp && "Singleton used outside of pattern.");
#endif
    }

private:
    static storage_type data_;
    static atomic<bool> initialized_;
    static mutex mu_;
};

template <typename T, size_t Size, size_t Alignment>
aligned_storage_t<Size, Alignment>
stack_singleton<T, Size, Alignment, true>::data_;

template <typename T, size_t Size, size_t Alignment>
atomic<bool>
stack_singleton<T, Size, Alignment, true>::initialized_ = ATOMIC_VAR_INIT(false);

template <typename T, size_t Size, size_t Alignment>
mutex
stack_singleton<T, Size, Alignment, true>::mu_;

PYCPP_END_NAMESPACE
