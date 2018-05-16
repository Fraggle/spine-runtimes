//
//  page.hpp
//  pool_allocator
//
//  Created by Mathieu Garaud on 06/05/2018.
//  Copyright © 2018 Pretty Simple. All rights reserved.
//

#ifndef SPINE_PAGE_HPP
#define SPINE_PAGE_HPP

#include "chunk.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

namespace spine
{
    class page final
    {
    public:
        using pointer = chunk::pointer;
    private:
        static constexpr std::size_t const SIZE = 4 * 1024;

        std::vector<chunk::value_type> _data;
        std::multimap<std::size_t, chunk> _free;
        std::unordered_map<pointer, chunk> _used;
        std::size_t _used_memory = 0;

    public:
        page() = default;
        explicit page(std::size_t size);
        page(page const&) = delete;
        page& operator=(page const&) = delete;
        page(page &&) noexcept = default;
        page& operator=(page &&) noexcept = delete;
        ~page() = default;

        pointer allocate(std::size_t n, std::size_t a);
        void deallocate(pointer p, std::size_t n);

        inline std::size_t used_size() const noexcept { return _used_memory; }

    private:
        void defrag();

        pointer find_greater(std::size_t n, std::size_t a);
        pointer find_perfect(std::size_t n, std::size_t a);
    };
}

#endif // SPINE_PAGE_HPP
