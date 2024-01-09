/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: image_data.h  
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

#ifndef __image_data_h__
#define __image_data_h__

#include "agile_vision/basic/base_type_def.h"

AGV_NAMESPACE_BEGIN

#ifdef _MSC_VER
    #pragma pack(push, 1)
#endif

#ifdef __GNUC__
    #define PRE_PACKED __attribute__ ((__packed__))
#else 
    #define PRE_PACKED
    #pragma pack(1) // For other compilers like MSVC
#endif

struct PRE_PACKED ImageData
{
    enum DataType
    {
        DT_UNK,
        DT_UCHAR,
        DT_USHORT,
        DT_INT,
        DT_FLOAT
    };
    uint8_t type = DT_UNK; 
    uint8_t channel = 1;
    uint8_t bits = 8;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 0;
    void* data = nullptr;
};

#ifdef _MSC_VER
    #pragma pack(pop)
#endif

AGV_NAMESPACE_END

#endif
