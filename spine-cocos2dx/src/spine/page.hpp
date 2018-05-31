//
//  page.hpp
//  Spine
//
//  Created by Mathieu Garaud on 06/05/2018.
//  Copyright © 2018 Pretty Simple. All rights reserved.
//

#ifndef SPINE_PAGE_HPP
#define SPINE_PAGE_HPP

#include <cstddef>
#include <cstdint>
#include <memory>

namespace spine
{
    class page final
    {
    public:
        static constexpr std::size_t const SIZE = 4 * 1024;
        using pointer = std::uint8_t*;
        using value_type = std::uint8_t;
    private:
        std::size_t const _size = 0;
        std::size_t _used_memory = 0;
        std::unique_ptr<value_type[]> const _data = nullptr;

    public:
        page() = delete;
        explicit page(std::size_t size);
        page(page const&) = delete;
        page& operator=(page const&) = delete;
        page(page &&) noexcept = delete;
        page& operator=(page &&) noexcept = delete;
        ~page() = default;

        pointer allocate(std::size_t n, std::size_t align);

        void deallocate_all();

        inline float usage() const noexcept { return static_cast<float>(_used_memory) / _size; }
    };
}

#endif // SPINE_PAGE_HPP
