//
// Copyright (c) 2018 Damian Jarek (damian dot jarek93 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/djarek/ufiber
//

#ifndef UFIBER_DETAIL_MONOTONIC_ALLOCATOR_HPP
#define UFIBER_DETAIL_MONOTONIC_ALLOCATOR_HPP

#include <ufiber/detail/config.hpp>

#include <cstddef>
#include <memory>

namespace ufiber
{
namespace detail
{

class memory_resource_base
{
public:
    virtual void* allocate(std::size_t size, std::size_t alignment) = 0;

    virtual void deallocate(void* p) noexcept = 0;

protected:
    memory_resource_base() = default;

    memory_resource_base(memory_resource_base&&) = delete;
    memory_resource_base(memory_resource_base const&) = delete;
    memory_resource_base& operator=(memory_resource_base&&) = delete;
    memory_resource_base& operator=(memory_resource_base const&) = delete;

    ~memory_resource_base() = default;
};

template<typename T>
class monotonic_allocator
{
public:
    using value_type = T;
    explicit monotonic_allocator(memory_resource_base& mr) noexcept
      : mr_{&mr}
    {
    }

    template<typename U>
    explicit monotonic_allocator(monotonic_allocator<U> const& other) noexcept
      : mr_{other.mr_}
    {
    }

    template<typename U>
    friend class monotonic_allocator;

    T* allocate(std::size_t n)
    {
        return static_cast<T*>(mr_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* p, std::size_t) noexcept
    {

        mr_->deallocate(p);
    }

private:
    memory_resource_base* mr_;
};

template<>
class monotonic_allocator<void>
{
public:
    using value_type = void;

    monotonic_allocator() = default;

    UFIBER_INLINE_DECL explicit monotonic_allocator(
      memory_resource_base& mr) noexcept;

    template<typename U>
    friend class monotonic_allocator;

private:
    memory_resource_base* mr_ = nullptr;
};

std::size_t constexpr compute_size(std::size_t N)
{
    return (N % alignof(std::max_align_t) == 0)
             ? N
             : N + sizeof(std::max_align_t) - N % alignof(std::max_align_t);
}

template<std::size_t N>
struct monotonic_memory_resource final : memory_resource_base
{
    static std::size_t constexpr storage_size = detail::compute_size(N);

    void* allocate(std::size_t size, std::size_t alignment) final
    {
        void* p = mem + storage_size - size_left;
        auto r = std::align(alignment, size, p, size_left);
        if (r)
        {
            size_left -= size;
            return p;
        }
        else
        {
            return ::operator new(size);
        }
    }

    void deallocate(void* p) noexcept final
    {
        if (std::less<void*>{}(mem + storage_size, p) ||
            std::less<void*>{}(p, mem))
        {
            ::operator delete(p);
        }
    }

    alignas(alignof(std::max_align_t)) char mem[storage_size];
    std::size_t size_left = N;
};

template<>
struct monotonic_memory_resource<0> final : memory_resource_base
{
    UFIBER_INLINE_DECL void* allocate(std::size_t size,
                                      std::size_t alignment) final;

    UFIBER_INLINE_DECL void deallocate(void* p) noexcept final;
};

} // detail
} // ufiber

#endif // UFIBER_DETAIL_MONOTONIC_ALLOCATOR_HPP
