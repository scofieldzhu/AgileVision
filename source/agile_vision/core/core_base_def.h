/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: core_base_def.h  
 *   Copyright (c) 2023-2023 scofieldzhu
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
#ifndef __core_base_def_h__
#define __core_base_def_h__

#include "agile_vision/basic/base_type_def.h"

AGV_NAMESPACE_BEGIN

enum class DataType
{
    kUnk,
    kInt,
    kFloat,
    kString,
    kBytes,
    kImage
};

inline bool IsFundamentalType(DataType t)
{
    return t == DataType::kInt || t == DataType::kFloat;
}

inline unsigned int GetFundamentalTypeSize(DataType t)
{
    if(t == DataType::kInt)
        return sizeof(int);
    if(t == DataType::kFloat)
        return sizeof(float);
    return 0;
}

enum class PinKind
{
    kNone,
    kIn,
    kOut
};

AGV_NAMESPACE_END

#endif
