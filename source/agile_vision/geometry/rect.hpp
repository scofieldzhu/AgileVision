 /*
 *  Ratel is a application framework, which provides some convenient librarys
 *  for for those c++ developers pursuing fast-developement.
 *  
 *  File: rect.hpp  
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
#ifndef __agile_vision_geometry_rect_h__
#define __agile_vision_geometry_rect_h__

#include "agile_vision/geometry/array_x.hpp"
#include "agile_vision/basic/portable_stream.h"

namespace agv {

    template <typename T>
    class Rect
    {
    public:
        using value_type = std::enable_if_t<std::is_same_v<T, int> || std::is_same_v<T, float>, T>;
        using point_type = ArrayX<value_type, 2>;
        using size_type = ArrayX<value_type, 2>;
        // 原生字段占位字节数（不含帧头），仅用于说明内存布局，不参与解析。
        static constexpr size_t kByteSize = point_type::kByteSize + size_type::kByteSize + kFloatSize;

        ByteVec serializeToBytes()const
        {
            ByteVec payload;
            detail::appendBytes(payload, lt_.serializeToBytes());
            detail::appendBytes(payload, size_.serializeToBytes());
            detail::writeScalar(payload, angle_);
            ByteVec framed;
            detail::writeFramedBytes(framed, detail::kRectMagic, payload);
            return framed;
        }

        size_t loadBytes(ConsBytePtr buffer, size_t size)
        {
            ConsBytePtr payload = nullptr;
            size_t payload_size = 0;
            if(!detail::readFramedBytes(buffer, size, detail::kRectMagic, payload, payload_size)){
                return 0;
            }
            ConsBytePtr cur = payload;
            size_t left = payload_size;
            point_type lt{value_type(0), value_type(0)};
            size_type sz{value_type(0), value_type(0)};
            auto used = lt.loadBytes(cur, left);
            if(used == 0 || used > left){
                return 0;
            }
            cur += used;
            left -= used;
            used = sz.loadBytes(cur, left);
            if(used == 0 || used > left){
                return 0;
            }
            cur += used;
            left -= used;
            float angle = 0.0f;
            if(!detail::readScalar(cur, left, angle) || left != 0){
                return 0;
            }
            lt_ = lt;
            size_ = sz;
            angle_ = angle;
            return detail::kPortableHeaderSize + payload_size;
        }

        void setLeftTop(const point_type& pt){ lt_ = pt; }
        const point_type& lt()const{ return lt_; }
        void setSize(const size_type& s){ size_ = s; }
        const auto& size()const{ return size_; }
        void setAngle(float a){ angle_ = a; }
        auto angle()const{ return angle_; }

        Rect(const point_type& pt, const size_type& size, float a)
            :lt_(pt),
            size_(size),
            angle_(a)
        {}

        Rect(value_type x, value_type y, value_type w, value_type h, float a)
            :lt_({x, y}),
            size_({w, h}),
            angle_(a)
        {}

    private:
        point_type lt_{value_type(0), value_type(0)};
        size_type size_{value_type(0), value_type(0)};
        float angle_ = 0.0f;
    };

}

#endif