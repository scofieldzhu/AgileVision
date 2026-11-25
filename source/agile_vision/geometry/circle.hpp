/* 
 *  Ratel is a application framework, which provides some convenient librarys
 *  for for those c++ developers pursuing fast-developement.
 *  
 *  File: circle.hpp  
 *  Copyright (c) 2024-2024 scofieldzhu
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
#ifndef __agile_vision_geometry_circle_h__
#define __agile_vision_geometry_circle_h__

#include "agile_vision/geometry/array_x.hpp"
#include "agile_vision/geometry/element_proxy.hpp"
#include "agile_vision/geometry/proxy_combine.hpp"

namespace agv {

    template <typename T>
    class Circle final
    {
    public:
        using value_type = std::enable_if_t<std::is_same_v<T, int> || std::is_same_v<T, float>, T>;
        using point_type = ArrayX<value_type, 2>;
        using float_proxy = ElementProxy<float, false>;

        ByteVec serializeToBytes()const
        {
            float_proxy fp(radius_);
            return ProxyCombine<point_type, float_proxy>::SerializeToBytes(center_, fp);
        }

        size_t loadBytes(ConsBytePtr buffer, size_t size)
        {
            if(buffer == nullptr)
            {
                return 0;
            }
            point_type center;
            float_proxy fp(0.0f);
            auto consumed = ProxyCombine<point_type, float_proxy>::LoadBytes(center, fp, buffer, size);
            if(consumed == 0)
            {
                return 0;
            }
            center_ = center;
            radius_ = fp.element();
            return consumed;
        }

        void setCenter(value_type x, value_type y){ center_ = point_type{x, y}; }
        void setCenter(const point_type& pt){ center_ = pt; }
        const point_type& center()const{ return center_; }
        void setRadius(float r){ radius_ = r; }
        auto radius()const{ return radius_; }

        Circle(const point_type& pt, float r)
            :center_(pt),
            radius_(r)
        {}

    private:
        point_type center_{value_type(0), value_type(0)};
        float radius_ = 0.0f;
    };

}

#endif
