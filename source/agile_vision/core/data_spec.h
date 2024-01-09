/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: data_spec.h  
 *   Copyright (c) 2023-2024 scofieldzhu
 *  
 *   MIT License
 *  
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *  
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *  
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#ifndef __data_spec_h__
#define __data_spec_h__

#include <cassert>
#include "agile_vision/core/core_export.h"
#include "agile_vision/core/core_base_def.h"

AGV_NAMESPACE_BEGIN

struct AGV_CORE_API DataSpec
{
    AgvBytes serializeToBytes()const;
    size_t loadBytes(ConsAgvBytePtr buffer, size_t size);
    static DataSpec Single(DataType type){ return {type}; }
    static DataSpec SingleInt(){ return StaticArray(DataType::kInt); }
    static DataSpec SingleFloat(){ return StaticArray(DataType::kFloat); }
    static DataSpec SingleString(){ return StaticArray(DataType::kString); }
    static DataSpec SingleBytes(){ return StaticArray(DataType::kBytes); }
    static DataSpec SingleImage(){ return StaticArray(DataType::kImage); }
    static DataSpec StaticArray(DataType type, unsigned int fixed_size = 1)
    { 
        assert(fixed_size);
        return {type, 0, fixed_size}; 
    }
    static DataSpec DynamicArray(DataType type){ return {type, 0, 0}; }
    bool isStaticArray()const{ return arry_size > 0; }
    bool isDynamicArray()const{ return arry_size == 0; }    
    bool compatibleWith(const DataSpec& ds)const;
    DataType major_type = DataType::kUnk;
    unsigned int subtype = 0;
    unsigned int arry_size = 1; // 0 means: size dynamic, other value means fix size
};

AGV_NAMESPACE_END

#endif

