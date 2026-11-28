 /*
 *  Ratel is a application framework, which provides some convenient librarys
 *  for for those c++ developers pursuing fast-developement.
 *  
 *  File: vec_proxy.hpp  
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
#ifndef __agile_vision_geometry_vec_proxy_h__
#define __agile_vision_geometry_vec_proxy_h__

#include <iterator>
#include <algorithm>
#include <type_traits>
#include "agile_vision/basic/string_proxy.h"
#include "agile_vision/basic/portable_stream.h"
#include "agile_vision/geometry/is_serializable.hpp"
#include "agile_vision/geometry/default_creator.hpp"

namespace agv {

    template <class E>
    concept VecProxySerializableMember = VecMemberSerializable<E, BytePtr, ConsBytePtr> && !std::same_as<E, StringProxy>;

    template <class E>
    concept VecProxyMember = VecProxySerializableMember<E> || std::is_arithmetic_v<E> || std::same_as<E, std::string>;

    template <class E, bool Ref = false>
    class VecProxy;

    template <class E>
    class VecProxy<E, false>
    {
    public:
        using element_type = std::remove_pointer_t<E>;
        using element_reference = E&;
        static constexpr bool kRef = false;
        static constexpr bool kIsPointer = std::is_pointer_v<E>;
        static constexpr bool kPlainElement = !std::is_arithmetic_v<element_type> && !std::same_as<element_type, std::string>;
        static_assert(VecProxyMember<element_type>, "E is not allowed element type for VecProxy!");
        static_assert(!kIsPointer || kPlainElement, "E is not allowed element type for VecProxy!");
        using list_type = std::vector<E>;
        using list_reference = std::vector<E>&;
        using list_const_reference = const std::vector<E>&;
        using default_creator = DefaultCreator<E>;

        static ByteVec SerializeToBytes(list_const_reference list)
        {
            ByteVec payload;
            detail::writeU32(payload, static_cast<uint32_t>(list.size()));
            if constexpr(std::is_arithmetic_v<element_type>){
                for(const auto& element : list){
                    detail::writeScalar(payload, element);
                }
            }else
            {
                for(const auto& v : list){
                    if constexpr(std::is_same_v<element_type, std::string>){
                        if constexpr(kIsPointer){
                            detail::appendBytes(payload, StringProxy(*v).serializeToBytes());
                        }else
                        {
                            detail::appendBytes(payload, StringProxy(v).serializeToBytes());
                        }
                    }else
                    {
                        if constexpr(kIsPointer){
                            detail::appendBytes(payload, v->serializeToBytes());
                        }else
                        {
                            detail::appendBytes(payload, v.serializeToBytes());
                        }
                    }
                }
            }
            ByteVec framed;
            detail::writeFramedBytes(framed, detail::kVecProxyMagic, payload);
            return framed;
        }

        ByteVec serializeToBytes()const
        {
            return SerializeToBytes(list_);
        }

        static size_t LoadBytes(list_reference list, ConsBytePtr byte_data, size_t size)
        {
            ConsBytePtr payload = nullptr;
            size_t payload_size = 0;
            if(!detail::readFramedBytes(byte_data, size, detail::kVecProxyMagic, payload, payload_size)){
                return 0;
            }
            ConsBytePtr cur = payload;
            size_t left = payload_size;
            uint32_t element_count = 0;
            if(!detail::readU32(cur, left, element_count)){
                return 0;
            }
            list.clear();
            if constexpr(std::is_arithmetic_v<element_type>){
                if(element_count > left / sizeof(element_type)){
                    return 0;
                }
                list.resize(element_count);
                for(size_t i = 0; i < element_count; ++i){
                    if(!detail::readScalar(cur, left, list[i])){
                        return 0;
                    }
                }
            }else
            {
                for(uint32_t i = 0; i < element_count; ++i){
                    E e = default_creator::Create();
                    size_t finish_size = 0;
                    if constexpr(std::is_same_v<element_type, std::string>){
                        StringProxy strp;
                        finish_size = strp.loadBytes(cur, left);
                        if constexpr(kIsPointer){
                            *e = strp.stdStr();
                        }else
                        {
                            e = strp.stdStr();
                        }
                    }else
                    {
                        if constexpr(kIsPointer){
                            finish_size = e->loadBytes(cur, left);
                        }else
                        {
                            finish_size = e.loadBytes(cur, left);
                        }
                    }
                    if(finish_size == 0 || finish_size > left){
                        return 0;
                    }
                    cur += finish_size;
                    left -= finish_size;
                    if constexpr(kIsPointer){
                        list.push_back(e);
                    }else
                    {
                        list.push_back(std::move(e));
                    }
                }
            }
            if(left != 0){
                return 0;
            }
            return detail::kPortableHeaderSize + payload_size;
        }

        size_t loadBytes(ConsBytePtr byte_data, size_t size)
        {
            return LoadBytes(list_, byte_data, size);
        }

        list_reference mutableData()
        {
            return list_;
        }

        list_const_reference data()const
        {
            return list_;
        }
    
        VecProxy()
        {}

    private:
        list_type list_;
    };

    template <class E>
    class VecProxy<E, true>
    {
    public:
        static constexpr bool kRef = true;
        using false_type = VecProxy<E, false>;
        using list_reference = std::vector<E>&;
        using list_const_reference = const std::vector<E>&;

        ByteVec serializeToBytes()const
        {
            return false_type::SerializeToBytes(list_);
        }

        size_t loadBytes(ConsBytePtr byte_data, size_t size)
        {
            return false_type::LoadBytes(list_, byte_data, size);
        }

        list_reference mutableData()
        {
            return list_;
        }

        list_const_reference data()const
        {
            return list_;
        }
    
        VecProxy(list_reference list)
            :list_(list)
        {}

        ~VecProxy()
        {}

    private:
        list_reference list_;
    };

}

#endif // __agile_vision_geometry_vec_proxy_h__