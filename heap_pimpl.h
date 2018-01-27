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
 //TODO: add the synopsis
 */

#pragma once

#include <pycpp/stl/cstddef.h>
#include <pycpp/stl/memory.h>
#include <pycpp/stl/utility.h>

PYCPP_BEGIN_NAMESPACE

// OBJECTS
// -------

// TODO: need allocator_destructor...


PYCPP_END_NAMESPACE

