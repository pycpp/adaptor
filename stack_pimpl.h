//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \addtogroup PyCPP
 *  \brief Fast PIMPL idiom without pointer overhead.
 *
 *  Fast PIMPL idiom using a pre-defined stack buffer of a custom
 *  size to avoid dynamic allocation. This avoids additional
 *  allocation required for PIMPL, enhancing performance.
 *  However, the type size **must** be known during template
 *  instantiation, and therefore loses the ABI (Application
 *  Binary Interface) compatibility PIMPL is frequently used
 *  for.
 *
 *  To avoid changes in class layout or size leading to undefined
 *  behavior, the destructor includes a static assert to ensure
 *  the alignment and type-size are compatible.
 *
 *  The class should be used as a private member variable encapsulating
 *  the implied class in the public class. For example:
 *
 *  \code
 *      #include <pycpp/adaptor/stack_pimpl.h>
 *
 *      struct file_impl;
 *      struct file
 *      {
 *      public:
 *      private:
 *          stack_pimpl<file_impl> impl_;
 *      };
 *
 *  The design is based off of Herb Sutter's GotW:
 *      http://www.gotw.ca/gotw/028.htm
 *
 *  The implementation is refined off of Malte Skarupke type-safe
 *  PIMPL implementation:
 *      https://probablydance.com/2013/10/05/type-safe-pimpl-implementation-without-overhead/
 *
 *  For the pitfalls on `aligned_storage`, read:
 *      https://whereswalden.com/tag/stdaligned_storage/
 *
 *  \synopsis
 *      template <
 *          typename T,
 *          size_t Size = sizeof(T),
 *          size_t Alignment = alignof(max_align_t)
 *      >
 *      class stack_pimpl
 *      {
 *      public:
 *          static constexpr size_t size = Size;
 *          static constexpr size_t alignment = Alignment;
 *
 *          using value_type = T;
 *          using reference = T&;
 *          using const_reference = const T&;
 *          using pointer = T*;
 *          using const_pointer = const T*;
 *
 *          stack_pimpl();
 *          stack_pimpl(const stack_pimpl& x);
 *          stack_pimpl& operator=(const stack_pimpl& x);
 *          stack_pimpl(stack_pimpl&& x);
 *          stack_pimpl& operator=(stack_pimpl&& x);
 *          stack_pimpl(const value_type& x);
 *          stack_pimpl& operator=(const value_type& x);
 *          stack_pimpl(value_type&& x);
 *          stack_pimpl& operator=(value_type&& x);
 *          ~stack_pimpl();
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
 *          void swap(stack_pimpl& x);
 *      };
 */

#pragma once

#include <pycpp/stl/cstddef.h>
#include <pycpp/stl/type_traits.h>
#include <pycpp/stl/utility.h>

PYCPP_BEGIN_NAMESPACE

namespace pimp_detail
{
// DETAIL
// ------


// Static check to ensure the type is properly sized and
// aligned to avoid any undefined behavior. Ensure
// the size is exactly equal, and that the alignment
// is at least as strict as the type alignment.
// Larger alignments are stricter on the memory locations
// they can be placed, and any stricter alignment can
// be used in place of a weaker one, according to the C standard.

template <typename T, size_t Size, size_t Alignment>
inline
void
assert_storage()
noexcept
{
    static_assert(sizeof(T) == Size, "");
    static_assert(alignof(T) <= Alignment, "");
}


template <typename T, size_t Size, size_t Alignment>
struct storage_asserter
{
    inline
    storage_asserter()
    noexcept
    {
        assert_storage<T, Size, Alignment>();
    }
};

}   /* pimp_detail */

// DECLARATION
// -----------

/**
 *  \brief PIMPL idiom using aligned storage to avoid dynamic allocation.
 */
template <
    typename T,
    size_t Size = sizeof(T),
    size_t Alignment = alignof(max_align_t)
>
class stack_pimpl
{
public:
    // MEMBER VARIABLES
    // ----------------
    static constexpr size_t size = Size;
    static constexpr size_t alignment = Alignment;

    // MEMBER TYPES
    // ------------
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

    // MEMBER FUNCTIONS
    // ----------------
    stack_pimpl()
    {
        new (&get()) value_type();
    }

    stack_pimpl(
        const stack_pimpl& x
    )
    {
        new (&get()) value_type(x.get());
    }

    stack_pimpl&
    operator=(
        const stack_pimpl& x
    )
    {
        if (this != &x) {
            get() = x.get();
        }
        return *this;
    }

    stack_pimpl(
        stack_pimpl&& x
    )
    {
        new (&get()) value_type(move(x.get()));
    }

    stack_pimpl&
    operator=(
        stack_pimpl&& x
    )
    {
        if (this != &x) {
            get() = move(x.get());
        }
        return *this;
    }

    stack_pimpl(
        const value_type& x
    )
    {
        new (&get()) value_type(x);
    }

    stack_pimpl&
    operator=(
        const value_type& x
    )
    {
        get() = x;
        return *this;
    }

    stack_pimpl(
        value_type&& x
    )
    {
        new (&get()) value_type(move(x));
    }

    stack_pimpl&
    operator=(
        value_type&& x
    )
    {
        get() = move(x);
        return *this;
    }

    ~stack_pimpl()
    {
        pimp_detail::storage_asserter<T, Size, Alignment> {};
        get().~T();
    }

    // CONVERSIONS
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
        return reinterpret_cast<reference>(mem_);
    }

    const_reference
    get()
    const noexcept
    {
        return reinterpret_cast<const_reference>(mem_);
    }

    // MODIFIERS
    void
    swap(
        stack_pimpl& x
    )
    {
        fast_swap(get(), x.get());
    }

private:
    using memory_type = aligned_storage_t<Size, Alignment>;
    memory_type mem_;
};

// SPECIALIZATION
// --------------

template <typename T, size_t Size, size_t Alignment>
struct is_relocatable<stack_pimpl<T, Size, Alignment>>: is_relocatable<T>
{};

// IMPLEMENTATION
// --------------

template <typename T, size_t Size, size_t Alignment>
const size_t stack_pimpl<T, Size, Alignment>::size;

template <typename T, size_t Size, size_t Alignment>
const size_t stack_pimpl<T, Size, Alignment>::alignment;

PYCPP_END_NAMESPACE
