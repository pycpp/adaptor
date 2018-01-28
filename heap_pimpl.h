//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \addtogroup PyCPP
 *  \brief Traditional heap-based PIMPL idiom.
 *
 *  Traditional PIMPL idiom using pointer indirection to avoid
 *  requiring knowledge of the type layout or size, reducing
 *  compile-time dependencies and allowing ABI (Application
 *  Binary Interface) compatiblity even if the original type's
 *  size or layout changes.
 *
 *  The heap PIMPL contains two classes: one for shared memory
 *  ownership, and one for unique memory ownership. With shared
 *  semantics, copying the wrapper shares the a reference to the
 *  implied member with the original class. With unique semantics,
 *  copying the wrapper creates a shallow copy of the implied
 *  member.
 *
 *  By default, the heap PIMPL wrappers use `allocator` for object
 *  construction and deletion, which adds overhead relative to
 *  `std::allocator`, when `USE_POLYMORPHIC_ALLOCATOR` is defined.
 *  Providing an empty allocator makes these classes as efficient
 *  as the underlying smart pointers.
 *
 *  The class should be used as a private member variable encapsulating
 *  the implied class in the public class. For example:
 *
 *  \code
 *      #include <pycpp/misc/heap_pimpl.h>
 *
 *      struct file_impl;
 *      struct file
 *      {
 *      public:
 *      private:
 *          unique_heap_pimpl<file_impl> impl_;
 *      };
 *
 *  \synopsis
 *      template <typename T, typename Allocator>
 *      class unique_heap_pimpl
 *      {
 *      public:
 *          // MEMBER TYPES
 *          // ------------
 *          using value_type = T;
 *          using reference = T&;
 *          using const_reference = const T&;
 *          using pointer = T*;
 *          using const_pointer = const T*;
 *          using allocator_type = Allocator;
 *          using traits_type = allocator_traits<allocator_type>;
 *          using deleter_type = implementation-defined;
 *          using storage_type = unique_ptr<value_type, deleter_type>;
 *
 *          unique_heap_pimpl();
 *          unique_heap_pimpl(const allocator_type& alloc);
 *          unique_heap_pimpl(const unique_heap_pimpl& x);
 *          unique_heap_pimpl(const unique_heap_pimpl& x, const allocator_type& alloc);
 *          unique_heap_pimpl(const value_type& x);
 *          unique_heap_pimpl(const value_type& x, const allocator_type& alloc);
 *          unique_heap_pimpl(unique_heap_pimpl&& x) noexcept;
 *          unique_heap_pimpl(unique_heap_pimpl&& x, const allocator_type& alloc) noexcept;
 *          unique_heap_pimpl(value_type&& x) noexcept;
 *          unique_heap_pimpl(value_type&& x, const allocator_type& alloc) noexcept;
 *          unique_heap_pimpl& operator=(const unique_heap_pimpl& x);
 *          unique_heap_pimpl& operator=(const value_type& x);
 *          unique_heap_pimpl& operator=(unique_heap_pimpl&& x) noexcept;
 *          unique_heap_pimpl& operator=(value_type&& x) noexcept;
 *
 *          reference operator*() noexcept;
 *          const_reference operator*() const noexcept;
 *          pointer operator->() noexcept;
 *          const_pointer operator->() const noexcept;
 *          operator reference() noexcept;
 *          operator const_reference() const noexcept;
 *          reference get() noexcept;
 *          const_reference get() const noexcept;
 *
 *          void swap(unique_heap_pimpl& x) noexcept;
 *      };
 *
 *      template <typename T, typename Allocator>
 *      void swap(unique_heap_pimpl<T, Allocator>& x, unique_heap_pimpl<T, Allocator>& y);
 *
 *
 *      template <typename T>
 *      class shared_heap_pimpl
 *      {
 *      public:
 *          using value_type = T;
 *          using reference = T&;
 *          using const_reference = const T&;
 *          using pointer = T*;
 *          using const_pointer = const T*;
 *          using storage_type = shared_ptr<value_type>;
 *
 *          shared_heap_pimpl();
 *
 *          template <typename Allocator>
 *          shared_heap_pimpl(const Allocator& alloc);
 *
 *          shared_heap_pimpl(const shared_heap_pimpl& x) = default;
 *          shared_heap_pimpl(const value_type& x);
 *
 *          template <typename Allocator>
 *          shared_heap_pimpl(const value_type& x, const Allocator& alloc);
 *
 *          shared_heap_pimpl(shared_heap_pimpl&& x) noexcept = default;
 *          shared_heap_pimpl(value_type&& x);
 *
 *          template <typename Allocator>
 *          shared_heap_pimpl(value_type&& x, const Allocator& alloc) noexcept;
 *
 *          shared_heap_pimpl& operator=(const shared_heap_pimpl&) = default;
 *          shared_heap_pimpl& operator=(shared_heap_pimpl&& x) noexcept = default;
 *          shared_heap_pimpl& operator=(const value_type& x);
 *          shared_heap_pimpl& operator=(value_type&& x) noexcept;
 *
 *          reference operator*() noexcept;
 *          const_reference operator*() const noexcept;
 *          pointer operator->() noexcept;
 *          const_pointer operator->() const noexcept;
 *          operator reference() noexcept;
 *          operator const_reference() const noexcept;
 *          reference get() noexcept;
 *          const_reference get() const noexcept;
 *
 *          void swap(shared_heap_pimpl& x) noexcept;
 *      };
 *
 *      template <typename T>
 *      void swap(shared_heap_pimpl<T>& x, shared_heap_pimpl<T>& y);
 */

#pragma once

#include <pycpp/stl/cassert.h>
#include <pycpp/stl/cstddef.h>
#include <pycpp/stl/memory.h>
#include <pycpp/stl/type_traits.h>
#include <pycpp/stl/utility.h>

PYCPP_BEGIN_NAMESPACE

// FORWARD
// -------

template <typename T, typename Allocator = allocator<T>>
class unique_heap_pimpl;

template <typename T>
class shared_heap_pimpl;

namespace
{
// HELPERS
// -------

template <typename T, typename Allocator, typename ... Ts>
unique_ptr<T, allocator_destructor<Allocator, 1>>
make_heap_pimpl(
    Ts&&... ts
)
{
    using deleter_type = allocator_destructor<Allocator, 1>;
    using traits_type = allocator_traits<Allocator>;
    using storage_type = unique_ptr<T, deleter_type>;

    Allocator a;
    auto ptr = storage_type(traits_type::allocate(a, 1), deleter_type(a));
    traits_type::construct(a, ptr.get(), forward<Ts>(ts)...);

    return ptr;
}


template <typename T, typename Allocator, typename ... Ts>
unique_ptr<T, allocator_destructor<Allocator, 1>>
allocate_heap_pimpl(
    const Allocator& alloc,
    Ts&&... ts
)
{
    using deleter_type = allocator_destructor<Allocator, 1>;
    using traits_type = allocator_traits<Allocator>;
    using storage_type = unique_ptr<T, deleter_type>;

    Allocator a(alloc);
    auto ptr = storage_type(traits_type::allocate(a, 1), deleter_type(a));
    traits_type::construct(a, ptr.get(), forward<Ts>(ts)...);

    return ptr;
}

}   /* anonymous */

// OBJECTS
// -------

// UNIQUE HEAP PIMPL

/**
 *  \brief PIMPL idiom using pointer indirection and unique semantics.
 */
template <typename T, typename Allocator>
class unique_heap_pimpl
{
public:
    // MEMBER TYPES
    // ------------
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using allocator_type = Allocator;
    using traits_type = allocator_traits<allocator_type>;
    using deleter_type = allocator_destructor<Allocator, 1>;
    using storage_type = unique_ptr<value_type, deleter_type>;

    // MEMBER FUNCTIONS
    // ----------------
    // Constructors
    unique_heap_pimpl():
        ptr_(make_heap_pimpl<value_type, allocator_type>())
    {}

    unique_heap_pimpl(
        const allocator_type& alloc
    ):
        ptr_(allocate_heap_pimpl<value_type>(alloc))
    {}

    // Copy constructors
    unique_heap_pimpl(
        const unique_heap_pimpl& x
    ):
        ptr_(make_heap_pimpl<value_type, allocator_type>(x.get()))
    {}

    unique_heap_pimpl(
        const unique_heap_pimpl& x,
        const allocator_type& alloc
    ):
        ptr_(allocate_heap_pimpl<value_type>(alloc, x.get()))
    {}

    unique_heap_pimpl(
        const value_type& x
    ):
        ptr_(make_heap_pimpl<value_type, allocator_type>(x))
    {}

    unique_heap_pimpl(
        const value_type& x,
        const allocator_type& alloc
    ):
        ptr_(allocate_heap_pimpl<value_type>(alloc, x))
    {}

    // Move constructors
    unique_heap_pimpl(
        unique_heap_pimpl&& x
    )
    noexcept:
        ptr_(make_heap_pimpl<value_type, allocator_type>(move(x.get())))
    {}

    unique_heap_pimpl(
        unique_heap_pimpl&& x,
        const allocator_type& alloc
    )
    noexcept:
        ptr_(allocate_heap_pimpl<value_type>(alloc, move(x.get())))
    {}

    unique_heap_pimpl(
        value_type&& x
    )
    noexcept:
        ptr_(make_heap_pimpl<value_type, allocator_type>(move(x)))
    {}

    unique_heap_pimpl(
        value_type&& x,
        const allocator_type& alloc
    )
    noexcept:
        ptr_(allocate_heap_pimpl<value_type>(alloc, move(x)))
    {}

    // Assignment
    unique_heap_pimpl&
    operator=(
        const unique_heap_pimpl& x
    )
    {
        if (this != &x) {
            get() = x.get();
        }
        return *this;
    }

    unique_heap_pimpl&
    operator=(
        const value_type& x
    )
    {
        get() = x;
        return *this;
    }

    unique_heap_pimpl&
    operator=(
        unique_heap_pimpl&& x
    )
    noexcept
    {
        if (this != &x) {
            ptr_ = move(x.ptr_);
        }
        return *this;
    }

    unique_heap_pimpl&
    operator=(
        value_type&& x
    )
    noexcept
    {
        get() = move(x);
        return *this;
    }

    // Observers
    reference
    operator*()
    noexcept
    {
        return get();
    }

    const_reference
    operator*()
    const noexcept
    {
        return get();
    }

    pointer
    operator->()
    noexcept
    {
        return &get();
    }

    const_pointer
    operator->()
    const noexcept
    {
        return &get();
    }

    operator
    reference()
    noexcept
    {
        return get();
    }

    operator
    const_reference()
    const noexcept
    {
        return get();
    }

    reference
    get()
    noexcept
    {
        return *ptr_;
    }

    const_reference
    get()
    const noexcept
    {
        return *ptr_;
    }

    // Modifiers
    void
    swap(
        unique_heap_pimpl& x
    )
    noexcept
    {
        using is_always_equal = typename traits_type::is_always_equal;
        return swap_impl(x, is_always_equal());
    }

private:
    storage_type ptr_;

    void
    swap_impl(
        unique_heap_pimpl& x,
        true_type
    )
    noexcept
    {
        fast_swap(ptr_, x.ptr_);
    }

    void
    swap_impl(
        unique_heap_pimpl& x,
        false_type
    )
    noexcept
    {
        // can only swap the allocators if they're equal
        auto& la = ptr_.get_deleter().get_allocator();
        auto& ra = x.ptr_.get_deleter().get_allocator();
        assert(la == ra && "Cannot swap if allocators are not equal.");

        value_type* lp = ptr_.release();
        value_type* rp = x.ptr_.release();
        ptr_.reset(rp);
        x.ptr_.reset(lp);
    }
};

template <typename T, typename Allocator>
void
swap(
    unique_heap_pimpl<T, Allocator>& x,
    unique_heap_pimpl<T, Allocator>& y
)
{
    return x.swap(y);
}

// SHARED HEAP PIMPL

/**
 *  \brief PIMPL idiom using pointer indirection and shared semantics.
 */
template <typename T>
class shared_heap_pimpl
{
public:
    // MEMBER TYPES
    // ------------
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using storage_type = shared_ptr<value_type>;

    // Constructors
    shared_heap_pimpl():
        ptr_(PYCPP_NAMESPACE::make_shared<value_type>())
    {}

    template <typename Allocator>
    shared_heap_pimpl(
        const Allocator& alloc
    ):
        ptr_(PYCPP_NAMESPACE::allocate_shared<value_type>(alloc))
    {}

    // Copy constructors
    shared_heap_pimpl(const shared_heap_pimpl& x) = default;

    shared_heap_pimpl(
        const value_type& x
    ):
        ptr_(PYCPP_NAMESPACE::make_shared<value_type>(x))
    {}

    template <typename Allocator>
    shared_heap_pimpl(
        const value_type& x,
        const Allocator& alloc
    ):
        ptr_(PYCPP_NAMESPACE::allocate_shared<value_type>(alloc, x))
    {}

    // Move constructors
    shared_heap_pimpl(shared_heap_pimpl&& x) noexcept = default;

    shared_heap_pimpl(
        value_type&& x
    )
    noexcept:
        ptr_(PYCPP_NAMESPACE::make_shared<value_type>(move(x)))
    {}

    template <typename Allocator>
    shared_heap_pimpl(
        value_type&& x,
        const Allocator& alloc
    )
    noexcept:
        ptr_(PYCPP_NAMESPACE::allocate_shared<value_type>(alloc, move(x)))
    {}

    // Assignment
    shared_heap_pimpl& operator=(const shared_heap_pimpl&) = default;
    shared_heap_pimpl& operator=(shared_heap_pimpl&& x) noexcept = default;

    shared_heap_pimpl&
    operator=(
        const value_type& x
    )
    {
        get() = x;
        return *this;
    }


    shared_heap_pimpl&
    operator=(
        value_type&& x
    )
    noexcept
    {
        get() = move(x);
        return *this;
    }

    // Observers
    reference
    operator*()
    noexcept
    {
        return get();
    }

    const_reference
    operator*()
    const noexcept
    {
        return get();
    }

    pointer
    operator->()
    noexcept
    {
        return &get();
    }

    const_pointer
    operator->()
    const noexcept
    {
        return &get();
    }

    operator
    reference()
    noexcept
    {
        return get();
    }

    operator
    const_reference()
    const noexcept
    {
        return get();
    }

    reference
    get()
    noexcept
    {
        return *ptr_;
    }

    const_reference
    get()
    const noexcept
    {
        return *ptr_;
    }

    // Modifiers
    void
    swap(
        shared_heap_pimpl& x
    )
    noexcept
    {
        ptr_.swap(x.ptr_);
    }

private:
    storage_type ptr_;
};

template <typename T>
void
swap(
    shared_heap_pimpl<T>& x,
    shared_heap_pimpl<T>& y
)
noexcept
{
    return x.swap(y);
}

PYCPP_END_NAMESPACE

