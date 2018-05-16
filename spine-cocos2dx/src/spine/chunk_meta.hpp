//
//  chunk_meta.hpp
//  NewYork
//
//  Created by Mathieu Garaud on 08/05/2018.
//  Copyright © 2018 Pretty Simple. All rights reserved.
//

#ifndef SPINE_CHUNK_META_HPP
#define SPINE_CHUNK_META_HPP

#include "page.hpp"

#include <cstddef>
#include <functional>

namespace spine
{
    struct chunk_meta final
    {
        std::reference_wrapper<spine::page> page;
        std::size_t size = 0;

        chunk_meta() = default;
        inline explicit chunk_meta(std::reference_wrapper<spine::page>&& p, std::size_t s) : page(std::move(p)), size(s) {}
        chunk_meta(chunk_meta const&) = delete;
        chunk_meta& operator=(chunk_meta const&) = delete;
        chunk_meta(chunk_meta &&) noexcept = default;
        chunk_meta& operator=(chunk_meta &&) noexcept = delete;
        ~chunk_meta() = default;
    };
}

#endif // SPINE_CHUNK_META_HPP
