//
//  chunk.hpp
//  pool_allocator
//
//  Created by Mathieu Garaud on 05/05/2018.
//  Copyright © 2018 Pretty Simple. All rights reserved.
//

#ifndef SPINE_CHUNK_HPP
#define SPINE_CHUNK_HPP

#include <cstddef>
#include <cstdint>

namespace spine
{
    struct chunk final
    {
        using value_type = std::uint8_t;
        using pointer = value_type*;

        pointer data = nullptr;
        std::size_t size = 0;

        chunk() = default;
        inline explicit chunk(pointer d, std::size_t s) : data(d), size(s) {}
        chunk(chunk const&) = delete;
        chunk& operator=(chunk const&) = delete;
        chunk(chunk &&) noexcept = default;
        chunk& operator=(chunk &&) noexcept = delete;
        ~chunk() = default;
    };
}

#endif // SPINE_CHUNK_HPP
