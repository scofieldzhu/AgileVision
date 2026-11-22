/* 
 *  Ratel is a application framework, which provides some convenient librarys
 *  for for those c++ developers pursuing fast-developement.
 *  
 *  File: basic_typedef.h  
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

#ifndef __agile_vision_basic_typedef_h__
#define __agile_vision_basic_typedef_h__

#include <cstdint>
#include <cassert>
#include <vector>
#include <memory>
#include <string>
#include <optional>

namespace agv {

    // ---------------------------------------------------------------------------
    // 基础字节类型
    // ---------------------------------------------------------------------------
    using Byte = char;
    using BytePtr = Byte*;
    using ConsBytePtr = const Byte*;
    using ByteVec = std::vector<Byte>;

    constexpr size_t kIntSize = sizeof(int);
    constexpr size_t kUIntSize = sizeof(unsigned int);
    constexpr size_t kFloatSize = sizeof(float);

    // ---------------------------------------------------------------------------
    // AgileVision 业务类型别名（合并自原 base_type_def.h）
    // ---------------------------------------------------------------------------
    using agile_vision_byte = Byte;
    using ConsAgvBytePtr = const Byte*;
    using AgvBytes = ByteVec;
    using AgvMultiBytes = std::vector<AgvBytes>;
    using AgvString = std::string;
    using AgvMultiString = std::vector<AgvString>;
    using agile_vision_time_t = uint64_t;

}

#endif // __agile_vision_basic_typedef_h__
