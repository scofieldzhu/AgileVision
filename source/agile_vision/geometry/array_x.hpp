/* 
 *  Ratel is a application framework, which provides some convenient librarys
 *  for for those c++ developers pursuing fast-developement.
 *  
 *  File: array_x.hpp  
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

#ifndef __agile_vision_geometry_array_x_h__
#define __agile_vision_geometry_array_x_h__

#include <type_traits>
#include <format>
#include <array>
#include <stdexcept>
#include <initializer_list>
#include "agile_vision/basic/basic_typedef.h"
#include "agile_vision/basic/portable_stream.h"

namespace agv {

    template <typename T, size_t S>
    class ArrayX final
    {
    public:
        static_assert(S, "Size must not be zero!");
        using value_type = std::enable_if_t<std::is_arithmetic_v<T>, T>;    
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;    
        static constexpr size_t kN = S;    
        using array_type = value_type[kN];
        // 原生值数组的字节数，用于内存布局说明；序列化帧另含魔数头。
        static constexpr size_t kByteSize = sizeof(value_type) * kN;

        ByteVec serializeToBytes()const
        {
            ByteVec payload;
            for(size_t i = 0; i < kN; ++i){
                detail::writeScalar(payload, arry_[i]);
            }
            ByteVec framed;
            detail::writeFramedBytes(framed, detail::kArrayXMagic, payload);
            return framed;
        }

        size_t loadBytes(ConsBytePtr buffer, size_t size)
        {
            ConsBytePtr payload = nullptr;
            size_t payload_size = 0;
            if(!detail::readFramedBytes(buffer, size, detail::kArrayXMagic, payload, payload_size)){
                return 0;
            }
            ConsBytePtr cur = payload;
            size_t left = payload_size;
            value_type loaded[kN] = {value_type(0)};
            for(size_t i = 0; i < kN; ++i){
                if(!detail::readScalar(cur, left, loaded[i])){
                    return 0;
                }
            }
            if(left != 0){
                return 0;
            }
            for(size_t i = 0; i < kN; ++i){
                arry_[i] = loaded[i];
            }
            return detail::kPortableHeaderSize + payload_size;
        }

        reference operator[](int idx)
        {
            if(idx < 0 || static_cast<size_t>(idx) >= kN){
                throw std::out_of_range(std::format("Invalid index value:{}, array size:{}!", idx, kN));
            }
            return this->arry_[static_cast<size_t>(idx)];
        }

        const_reference operator[](int idx)const
        {
            return const_cast<ArrayX&>(*this).operator[](idx);
        }

        std::array<value_type, kN> toArray()const
        {
            return std::to_array(this->arry_);
        }

        pointer data()
        {
            return this->arry_;
        }

        const_pointer data()const
        {
            return this->arry_;
        }

        array_type arry_ = {value_type(0)};
    
    };

}

#endif