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
#include "agile_vision/basic/portable_stream.h"
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
            ByteVec payload;
            detail::writeU32(payload, static_cast<uint32_t>(map.size()));
            detail::appendBytes(payload, keys.serializeToBytes());
            detail::appendBytes(payload, values.serializeToBytes());
            ByteVec framed;
            detail::writeFramedBytes(framed, detail::kDictProxyMagic, payload);
            return framed;
        }

        ByteVec serializeToBytes()const
        {
            return SerializeToBytes(map_);
        }

        static size_t LoadBytes(map_reference map, ConsBytePtr byte_data, size_t size)
        {
            ConsBytePtr payload = nullptr;
            size_t payload_size = 0;
            if(!detail::readFramedBytes(byte_data, size, detail::kDictProxyMagic, payload, payload_size)){
                return 0;
            }
            ConsBytePtr cur = payload;
            size_t left = payload_size;
            uint32_t number = 0;
            if(!detail::readU32(cur, left, number)){
                return 0;
            }
            key_vec_proxy_type keys;
            const auto finish_bytes = keys.loadBytes(cur, left);
            if(finish_bytes == 0 || finish_bytes > left){
                return 0;
            }
            left -= finish_bytes;
            cur += finish_bytes;

            value_vec_proxy_type values;
            const auto values_bytes = values.loadBytes(cur, left);
            if(values_bytes == 0 || values_bytes > left){
                return 0;
            }
            left -= values_bytes;
            cur += values_bytes;

            if(left != 0){
                return 0;
            }
            // 条目数不得超过实际解析出的键/值数量，避免越界访问。
            if(number > keys.data().size() || number > values.data().size()){
                return 0;
            }
            map.clear();
            for(uint32_t i = 0; i < number; ++i){
                map.insert({keys.data()[i], values.data()[i]});
            }
            return detail::kPortableHeaderSize + payload_size;
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