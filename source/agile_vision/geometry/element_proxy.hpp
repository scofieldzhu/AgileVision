/* 
 *  Ratel is a application framework, which provides some convenient librarys
 *  for for those c++ developers pursuing fast-developement.
 *  
 *  File: element_proxy.hpp  
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

#ifndef __agile_vision_geometry_element_proxy_h__
#define __agile_vision_geometry_element_proxy_h__

#include <type_traits>
#include "agile_vision/basic/string_proxy.h"
#include "agile_vision/basic/portable_stream.h"

namespace agv {

    template <typename T, bool Ref = false>
    class ElementProxy
    {
    public:
        static_assert(std::is_arithmetic_v<T> && sizeof(T) >= 4, "T is not allowed element type!");
        static constexpr bool kRef = Ref;
        using value_type = T;
        using member_value_type = std::conditional_t<kRef, T&, T>;
        using reference = value_type&;
        using const_reference = const value_type&;

        ByteVec serializeToBytes()const
        {
            ByteVec payload;
            detail::writeScalar(payload, element_);
            ByteVec framed;
            detail::writeFramedBytes(framed, detail::kElementProxyMagic, payload);
            return framed;
        }

        size_t loadBytes(ConsBytePtr buffer, size_t size)
        {
            ConsBytePtr payload = nullptr;
            size_t payload_size = 0;
            if(!detail::readFramedBytes(buffer, size, detail::kElementProxyMagic, payload, payload_size)){
                return 0;
            }
            ConsBytePtr cur = payload;
            size_t left = payload_size;
            value_type loaded = value_type(0);
            if(!detail::readScalar(cur, left, loaded) || left != 0){
                return 0;
            }
            element_ = loaded;
            return detail::kPortableHeaderSize + payload_size;
        }

        reference mutableElement()
        { 
            return element_;
        }

        const_reference element()const
        {
            return element_;
        }
    
        ElementProxy(member_value_type v)
            :element_(v){}

        ~ElementProxy()
        {}

    private:
        member_value_type element_;
    };

}

#endif