//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \brief Generic singleton adaptor for C++ classes.
 */

#pragma once

#include <pycpp/config.h>
#include <pycpp/stl/atomic.h>
#include <pycpp/stl/memory.h>
#include <pycpp/stl/mutex.h>
#include <pycpp/stl/type_traits.h>
#include <pycpp/stl/utility.h>

PYCPP_BEGIN_NAMESPACE

// OBJECTS
// -------


/**
 *  \brief Optionally thread-safe heap singleton pattern.
 *
 *  Use `dummy_mutex` to disable thread-safety. Since the object is
 *  only allocated a single time, the global allocator is sufficient.
 */
template <typename T, bool ThreadSafe>
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
        return data_;
    }

protected:
    heap_singleton() = default;
    heap_singleton(const heap_singleton&) = delete;
    heap_singleton& operator=(const heap_singleton&) = delete;

    ~heap_singleton()
    {
        delete data_;
    }

private:
    static value_type* data_;
};

template <typename T>
T* heap_singleton<T, false>::data_ = nullptr;


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
        return p;
    }

protected:
    heap_singleton() = default;
    heap_singleton(const heap_singleton&) = delete;
    heap_singleton& operator=(const heap_singleton&) = delete;

    ~heap_singleton()
    {
        delete data_.load(memory_order_acquire);
    }

private:
    static atomic<value_type*> data_;
    static mutex mu_;
};

template <typename T>
atomic<T*> heap_singleton<T, true>::data_ = ATOMIC_VAR_INIT(nullptr);


/**
 *  \brief Optionally thread-safe stack singleton pattern.
 *
 *  Use `dummy_mutex` to disable thread-safety. Since the object is
 *  only allocated a single time, the global allocator is sufficient.
 */
template <typename T, bool ThreadSafe>
class stack_singleton;

// Single-threaded
template <typename T>
class stack_singleton<T, false>
{
public:
    static constexpr bool thread_safe = false;
    using value_type = T;

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
        value_type& r = reinterpret_cast<value_type&>(data_);
        if (initialized_) {
            r.~T();
        }
    }

private:
    using storage_type = aligned_storage_t<sizeof(T), alignof(T)>;

    static storage_type data_;
    static bool initialized_;
};

template <typename T>
bool stack_singleton<T, false>::initialized_ = false;


// Multi-threaded
template <typename T>
class stack_singleton<T, true>
{
public:
    static constexpr bool thread_safe = true;
    using value_type = T;

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
        value_type& r = reinterpret_cast<value_type&>(data_);
        if (initialized_.load()) {
            r.~T();
        }
    }

private:
    using storage_type = aligned_storage_t<sizeof(T), alignof(T)>;

    static storage_type data_;
    static atomic<bool> initialized_;
    static mutex mu_;
};

template <typename T>
atomic<bool> stack_singleton<T, true>::initialized_ = ATOMIC_VAR_INIT(false);

PYCPP_END_NAMESPACE
