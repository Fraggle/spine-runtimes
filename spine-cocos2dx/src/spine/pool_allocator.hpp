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

#include <deque>
#include <type_traits>

namespace spine
{
    template <typename T>
    class pool_allocator final
    {
    public:
        using pointer = T*;

    private:
        std::deque<page> _small;
        std::deque<page> _large;
        std::deque<page> _huge;

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
            if (n <= (1024 * 4))
            {
                return _allocate(_small, n);
            }
            else if (n > (1024 * 4) && n <= (1024 * 1024 * 4))
            {
                return _allocate(_large, n);
            }
            else
            {
                return _allocate(_huge, n);
            }
        }

        pointer _allocate(decltype(_small)& data, std::size_t n)
        {
            for (auto& page : data)
            {
                if (pointer ret = reinterpret_cast<pointer>(page.allocate(n * sizeof(T), alignof(T))); ret != nullptr)
                {
                    auto it = _pointers.emplace(ret, chunk_meta{std::ref(page), n});
                    assert(it.second == true);
                    return ret;
                }
            }

            data.emplace_back(page{n});
            pointer ret = reinterpret_cast<pointer>(data.back().allocate(n * sizeof(T), alignof(T)));
            auto it = _pointers.emplace(ret, chunk_meta{std::ref(data.back()), n});
            assert(it.second == true);
            return ret;
        }
    };
}

#endif // SPINE_POOL_ALLOCATOR_HPP
