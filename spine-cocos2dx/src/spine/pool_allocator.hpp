//
//  pool_allocator.hpp
//  pool_allocator
//
//  Created by Mathieu Garaud on 06/05/2018.
//  Copyright © 2018 Pretty Simple. All rights reserved.
//

#ifndef SPINE_POOL_ALLOCATOR_HPP
#define SPINE_POOL_ALLOCATOR_HPP

#include "chunk_meta.hpp"
#include "page.hpp"

#include <cassert>
#include <list>
#include <type_traits>

namespace spine
{
    template <typename T>
    class pool_allocator final
    {
    public:
        using pointer = T*;

    private:
        std::list<page> _pages;
        std::unordered_map<pointer, chunk_meta> _pointers;

    public:
        pointer allocate(std::size_t n)
        {
            auto ret = _allocate(n);
            if constexpr(std::is_default_constructible<T>::value)
            {
                for (std::size_t i = 0; i < n; ++i)
                {
                    new (ret+i) T();
                }
            }
            return ret;
        }

        void deallocate(pointer p, std::size_t n)
        {
            if (auto search = _pointers.find(p); search != _pointers.end())
            {
                if constexpr(std::is_destructible<T>::value)
                {
                    for (std::size_t i = 0; i < search->second.size; ++i)
                    {
                        (p+i)->~T();
                    }
                }
                search->second.page.get().deallocate(reinterpret_cast<page::pointer>(p), search->second.size * sizeof(T));
                _pointers.erase(search);
            }
        }

        void deallocate_all()
        {
            for (auto& d : _pointers)
            {
                if constexpr(std::is_destructible<T>::value)
                {
                    for (std::size_t i = 0; i < d.second.size; ++i)
                    {
                        (d.first+i)->~T();
                    }
                }
                d.second.page.get().deallocate(reinterpret_cast<page::pointer>(d.first), d.second.size * sizeof(T));
            }
            _pointers.clear();
        }

    private:
        pointer _allocate(std::size_t n)
        {
            for (auto& page : _pages)
            {
                if (pointer ret = reinterpret_cast<pointer>(page.allocate(n * sizeof(T), alignof(T))); ret != nullptr)
                {
                    auto it = _pointers.emplace(ret, chunk_meta{std::ref(page), n});
                    assert(it.second == true);
                    return ret;
                }
            }

            _pages.emplace_back(page{n * sizeof(T)});
            auto& page = _pages.back();
            pointer ret = reinterpret_cast<pointer>(page.allocate(n * sizeof(T), alignof(T)));
            auto it = _pointers.emplace(ret, chunk_meta{std::ref(page), n});
            assert(it.second == true);
            return ret;
        }
    };
}

#endif // SPINE_POOL_ALLOCATOR_HPP
