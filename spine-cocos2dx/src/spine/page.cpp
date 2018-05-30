//
//  page.cpp
//  Spine
//
//  Created by Mathieu Garaud on 30/05/2018.
//  Copyright © 2018 Pretty Simple. All rights reserved.
//

#include "page.hpp"

inline std::uint8_t* next_aligned(std::uint8_t* p, std::size_t a) noexcept
{
    return reinterpret_cast<std::uint8_t*>((reinterpret_cast<std::uintptr_t>(p) + reinterpret_cast<std::uintptr_t>(a - 1)) & ~(static_cast<std::uintptr_t>(a - 1)));
}

inline constexpr std::size_t multiple_of(std::size_t p, std::size_t a) noexcept
{
    return ((p + (a - 1)) & ~(a - 1));
}

namespace spine
{

    constexpr std::size_t const page::SIZE;

    page::page(std::size_t size)
    : _size(multiple_of(size, SIZE))
    , _used_memory(0)
    , _data(std::make_unique<value_type[]>(_size))
    {
    }

    page::pointer page::allocate(std::size_t n, std::size_t align)
    {
        auto ret = reinterpret_cast<page::pointer>(_data.get()) + _used_memory;
        auto aligned_ptr = next_aligned(ret, align);
        std::uintptr_t const alocated_byte_size = (aligned_ptr - ret) + n;
        if ((_used_memory + alocated_byte_size) < _size)
        {
            _used_memory += alocated_byte_size;
            return aligned_ptr;
        }
        return nullptr;
    }

    void page::deallocate_all()
    {
        _used_memory = 0;
    }

}
