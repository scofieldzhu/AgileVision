/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: data_buffer.h  
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

#ifndef __data_buffer_h__
#define __data_buffer_h__

#include "agile_vision/core/data_spec.h"
#include "agile_vision/core/core_export.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API DataBuffer
{
public:
    AgvBytes serializeToBytes()const;
    size_t loadBytes(ConsAgvBytePtr buffer, size_t size);
    auto dataSpec()const{ return ds_; }
    size_t valueSize()const;
    bool isFixedSize()const{ return ds_.isStaticArray(); }
    bool isDynamicSize()const{ return !isFixedSize(); }
    bool resize(size_t number);
    void erase(size_t idx);
    void setIntValue(int v, size_t idx = 0);
    void setIntValue(const int* vp, size_t size);
    std::optional<int> getIntValue(size_t idx = 0)const;
    int* getIntPointer();
    const int* getIntPointer()const;
    void setFloatValue(float v, size_t idx = 0);
    void setFloatValue(const float* v, size_t size);
    std::optional<float> getFloatValue(size_t idx = 0)const;
    float* getFloatPointer();
    const float* getFloatPointer()const;
    void setImageValue(const ImageData& v, size_t idx = 0);
    std::optional<ImageData> getImageValue(size_t idx = 0)const;
    ImageData* getImagePointer();
    const ImageData* getImagePointer()const;
    void setStringValue(const char* s, size_t idx = 0);
    void setStringValue(const AgvMultiString& strs);
    const char* getStringValue(size_t idx = 0)const;    
    AgvMultiString getAllStringValues()const;    
    void setBytesValue(const agv_byte* bytes, size_t byte_size, size_t idx = 0);
    std::optional<AgvBytes> getBytesValue(size_t idx = 0)const;
    const AgvMultiBytes& getAllBytesValues()const;
    void updateModifyTime(agv_time_t t);
    agv_time_t lastModifyTime()const{ return last_modify_time_; }
    explicit DataBuffer(const DataSpec& data_spec);
    ~DataBuffer();

private:
    size_t getSize()const;
    DataSpec ds_;
    AgvBytes fundamental_bytes_;
    AgvMultiBytes bytes_table_;
    size_t value_size_ = 0;
    agv_time_t last_modify_time_ = 0;
};

AGV_NAMESPACE_END

#endif