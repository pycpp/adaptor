//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \brief Generic singleton adaptor for C++ classes.
 */

#pragma once

#include <pycpp/config.h>
#include <pycpp/stl/memory.h>

PYCPP_BEGIN_NAMESPACE

// OBJECTS
// -------

// TODO: lols, how do I do this generally???
// I want to remove boilerplate....

template <typename T>
class stack_singleton: public T
{
public:
    stack_singleton() = delete;
    stack_singleton(const stack_singleton&) = delete;
    stack_singleton& operator(const stack_singleton&) = delete;
};

//template <typename T, typename Allocator>
//class singleton
//{
//public:
//    using pointer = T*;
//    using element_type = T;
//    using deleter_type = Deleter;
//    // TODO: here...
//    // TODO: get...
//
//    template <typename ... Ts>
//    void
//    reset(
//        Ts&&... ts
//    )
//    {
//        ptr_.reset();
////        ptr_ = make_unique<T, Deleter>()
//    }
//
//    explicit
//    operator bool()
//    const
//    {
//        return bool(ptr_);
//    }
//
//private:
//    unique_ptr<T> ptr_;
//    // TODO: need Allocator
//};


PYCPP_END_NAMESPACE
