/* 
 *  Ratel is a application framework, which provides some convenient librarys
 *  for for those c++ developers pursuing fast-developement.
 *  
 *  File: dict_proxy.hpp  
 *  Copyright (c) 2023-2023 scofieldzhu
 *  
 *  MIT License
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef __agile_vision_geometry_dict_proxy_h__
#define __agile_vision_geometry_dict_proxy_h__

#include <map>
#include <cassert>
#include "agile_vision/basic/string_proxy.h"
#include "agile_vision/geometry/vec_proxy.hpp"

namespace agv {

    template <typename T>
    concept ArithStringType = std::is_arithmetic_v<T> || std::same_as<T, std::string>;

    template <class K, class V, bool Ref = false>
    class DictProxy;

    template <class K, class V>
    class DictProxy<K, V, false>
    {
    public:
        static_assert(ArithStringType<K>);
        static_assert(VecProxyMember<V>);
        static constexpr bool kRef = false;
        using key_type = K;
        using key_vec_proxy_type = VecProxy<key_type, false>;
        using value_type = V;
        using value_vec_proxy_type = VecProxy<value_type, false>;
        using map_type = std::map<K, V>;
        using map_reference = std::map<K, V>&;
        using const_map_reference = const std::map<K, V>&;

        static ByteVec SerializeToBytes(const_map_reference map)
        {
            key_vec_proxy_type keys;
            value_vec_proxy_type values;
            for(const auto& kv : map){
                keys.mutableData().push_back(kv.first);
                values.mutableData().push_back(kv.second);
            }
            ByteVec bv_k = keys.serializeToBytes();
            ByteVec bv_v = values.serializeToBytes();
            size_t total_size = bv_k.size() + bv_v.size() + kUIntSize * 2;
            ByteVec bv(total_size);
            unsigned int number = (unsigned int)map.size();
            auto cur_data = bv.data();
            memcpy(cur_data, &number, kUIntSize);
            cur_data += kUIntSize;
            unsigned int key_byte_size = (unsigned int)bv_k.size();
            memcpy(cur_data, &key_byte_size, kUIntSize);
            cur_data += kUIntSize;
            memcpy(cur_data, bv_k.data(), bv_k.size());
            cur_data += bv_k.size();
            memcpy(cur_data, bv_v.data(), bv_v.size());
            return bv;
        }

        ByteVec serializeToBytes()const
        {
            return SerializeToBytes(map_);
        }

        static size_t LoadBytes(map_reference map, ConsBytePtr byte_data, size_t size)
        {
            if(byte_data == nullptr || size < 2 * kUIntSize)
            {
                return 0;        
            }
            auto cur_data = byte_data;
            unsigned int number = 0;
            memcpy(&number, cur_data, kUIntSize);
            cur_data += kUIntSize;
            size_t left_size = size - kUIntSize;

            if(left_size < kUIntSize)
            {
                return 0;
            }
            unsigned int key_byte_size = 0;
            memcpy(&key_byte_size, cur_data, kUIntSize);
            cur_data += kUIntSize;
            left_size -= kUIntSize;

            if(key_byte_size > left_size)
            {
                return 0;
            }
            key_vec_proxy_type keys;
            const auto finish_bytes = keys.loadBytes(cur_data, key_byte_size);
            if(finish_bytes == 0 || finish_bytes > left_size)
            {
                return 0;
            }
            left_size -= finish_bytes;
            cur_data += finish_bytes;

            value_vec_proxy_type values;
            auto values_bytes = values.loadBytes(cur_data, left_size);
            if(values_bytes == 0 || values_bytes > left_size)
            {
                return 0;
            }
            left_size -= values_bytes;
            cur_data += values_bytes;

            // 条目数不得超过实际解析出的键/值数量，避免 at() 抛越界异常
            if(number > keys.data().size() || number > values.data().size())
            {
                return 0;
            }
            map.clear();
            for(unsigned int i = 0; i < number; ++i)
            {
                map.insert({keys.data()[i], values.data()[i]});
            }
            return size - left_size;
        }

        size_t loadBytes(ConsBytePtr byte_data, size_t size)
        {        
            return LoadBytes(map_, byte_data, size);
        }

        map_reference mutableData()
        {
            return map_;
        }

        const_map_reference data()const
        {
            return map_;
        }
    
        DictProxy()
        {}

        ~DictProxy()
        {}

    private:
        map_type map_;
    };

    template <class K, class V>
    class DictProxy<K, V, true>
    {
    public:
        static_assert(ArithStringType<K>);
        static_assert(VecProxyMember<V>);
        static constexpr bool kRef = true;
        using key_type = K;
        using key_vec_proxy_type = VecProxy<key_type, false>;
        using value_type = V;
        using value_vec_proxy_type = VecProxy<value_type, false>;
        using map_type = std::map<K, V>;
        using map_reference = std::map<K, V>&;
        using const_map_reference = const std::map<K, V>&;

        ByteVec serializeToBytes()const
        {
            return DictProxy<K, V, false>::SerializeToBytes(map_);
        }

        size_t loadBytes(ConsBytePtr byte_data, size_t size)
        {        
            return DictProxy<K, V, false>::LoadBytes(map_, byte_data, size);
        }

        map_reference mutableData()
        {
            return map_;
        }

        const_map_reference data()const
        {
            return map_;
        }
    
        DictProxy(map_reference m)
            :map_(m)
        {}

        ~DictProxy()
        {}

    private:
        map_reference map_;
    };

}

#endif