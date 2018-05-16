//
//  page.cpp
//  NewYork
//
//  Created by Mathieu Garaud on 09/05/2018.
//  Copyright © 2018 Pretty Simple. All rights reserved.
//

#include "page.hpp"

#include <cassert>
#include <iterator>
#include <set>

namespace spine
{
    inline bool is_aligned(std::uint8_t* p, std::size_t a) noexcept
    {
        return (reinterpret_cast<std::uintptr_t>(p) & reinterpret_cast<std::uintptr_t>(a - 1)) == 0;
    }

    inline std::uint8_t* next_aligned(std::uint8_t* p, std::size_t a) noexcept
    {
        return reinterpret_cast<std::uint8_t*>((reinterpret_cast<std::uintptr_t>(p) + reinterpret_cast<std::uintptr_t>(a - 1)) & ~(static_cast<std::uintptr_t>(a - 1)));
    }

    inline constexpr std::size_t multiple_of(std::size_t p, std::size_t a) noexcept
    {
        return ((p + (a - 1)) & ~(a - 1));
    }

    page::page(std::size_t size) : _data(multiple_of(size, SIZE), 0)
    {
        _free.emplace(_data.size(), chunk{_data.data(), _data.size()});
    }

    page::pointer page::allocate(std::size_t n, std::size_t a)
    {
        if (auto ret = find_perfect(n, a); ret != nullptr)
        {
            return ret;
        }

        if (auto ret = find_greater(n, a); ret == nullptr && static_cast<float>(_used_memory) < (_data.size() * 0.80f))
        {
            defrag();
            return find_greater(n, a);
        }
        else
        {
            return ret;
        }

        return nullptr;
    }

    void page::deallocate(page::pointer p, std::size_t n)
    {
        if (auto search = _used.find(p); search != _used.end())
        {
            auto ret = search->second.size;
            assert(ret == n);
            auto val = std::move(search->second);
            _used.erase(search);
            _free.emplace(ret, std::move(val));
            assert(_used_memory >= ret);
            _used_memory -= ret;
        }
    }

    page::pointer page::find_perfect(std::size_t n, std::size_t a)
    {
        if (auto search = _free.equal_range(n); search.first != search.second)
        {
            auto perfect = std::find_if(search.first, search.second, [a](auto const& pair) {
                return is_aligned(pair.second.data, a);
            });
            if (perfect != search.second)
            {
                auto ret = perfect->second.data;
                auto val = std::move(perfect->second);
                _free.erase(perfect);
                _used.emplace(ret, std::move(val));
                assert(_used_memory <= (_data.size() - n));
                _used_memory += n;
                return ret;
            }
        }

        return nullptr;
    }

    page::pointer page::find_greater(std::size_t n, std::size_t a)
    {
        if (auto upper = _free.upper_bound(n); upper != _free.end())
        {
            auto almost = std::find_if(upper, _free.end(), [n, a](auto const& pair) {
                return is_aligned(pair.second.data, a) || (next_aligned(pair.second.data, a) + n) <= (pair.second.data + pair.second.size);
            });
            if (almost != _free.end())
            {
                auto ret = almost->second.data;
                auto aligned_ptr = next_aligned(ret, a);
                auto val = std::move(almost->second);
                std::uintptr_t const distance = aligned_ptr - ret;
                if (distance > 0)
                {
                    _free.emplace(distance, chunk{ret, distance});
                }
                if ((val.size - distance - n) > 0)
                {
                    _free.emplace(val.size - distance - n, chunk{val.data + distance + n, val.size - distance - n});
                }
                assert(aligned_ptr == val.data + distance);
                val.data = aligned_ptr;
                val.size = n;
                _free.erase(almost);
                _used.emplace(aligned_ptr, std::move(val));
                assert(_used_memory <= (_data.size() - n));
                _used_memory += n;
                return aligned_ptr;
            }
        }

        return nullptr;
    }

    struct defrag_less
    {
        constexpr bool operator()(std::multimap<std::size_t, chunk>::iterator const& lhs, std::multimap<std::size_t, chunk>::iterator const& rhs ) const
        {
            return lhs->second.data < rhs->second.data;
        }
    };

    void page::defrag()
    {
        if (_free.size() > 1)
        {
            std::set<decltype(_free)::iterator, defrag_less> tmp;
            for(auto it = _free.begin(); it != _free.end(); ++it)
            {
                tmp.emplace(it);
            }

            for (auto it = tmp.begin(); it != std::prev(tmp.end());)
            {
                if (auto jt = std::next(it); ((*it)->second.data + (*it)->second.size) == (*jt)->second.data)
                {
                    (*jt)->second.data = (*it)->second.data;
                    (*jt)->second.size += (*it)->second.size;
                    _free.erase(*it);
                    it = tmp.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            for (auto it = _free.begin(); it != _free.end();)
            {
                if (it->second.size != it->first)
                {
                    _free.emplace(it->second.size, std::move(it->second));
                    it = _free.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }
}
