/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: data_buffer.cpp  
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

#include "data_buffer.h"
#include "spdlog/spdlog.h"

AGV_NAMESPACE_BEGIN

DataBuffer::DataBuffer(const DataSpec& spec)
    :ds_(spec)
{
    if(isFixedSize()){
        if(IsFundamentalType(ds_.major_type)){
            fundamental_bytes_.resize(ds_.arry_size * GetFundamentalTypeSize(ds_.major_type), 0);
        }else{
            bytes_table_.resize(ds_.arry_size);
        }        
    }
    value_size_ = getSize();
}

DataBuffer::~DataBuffer()
{
}

size_t DataBuffer::valueSize() const
{
    return value_size_;
}

size_t DataBuffer::getSize() const
{
    return IsFundamentalType(ds_.major_type) ? fundamental_bytes_.size() : bytes_table_.size();
}

bool DataBuffer::resize(size_t number)
{
    if(isFixedSize())
        return false;
    value_size_ = number;
    if(IsFundamentalType(ds_.major_type)){
        fundamental_bytes_.resize(value_size_ * GetFundamentalTypeSize(ds_.major_type), 0);
    }else{
        bytes_table_.resize(value_size_);
    }   
    return true;
}

void DataBuffer::erase(size_t idx)
{
    if(isFixedSize()){
        SPDLOG_ERROR("Fixed data buffer cannot execute erase operation!");
        return;
    }
    if(idx >= value_size_){
        SPDLOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
        return;
    }
    if(IsFundamentalType(ds_.major_type)){
        const size_t type_size = GetFundamentalTypeSize(ds_.major_type);
        auto start_iter = fundamental_bytes_.begin() + type_size * idx;
        auto last_iter = start_iter + type_size;
        fundamental_bytes_.erase(start_iter, last_iter);
    }else{
        bytes_table_.erase(bytes_table_.begin() + idx);
    }
    value_size_ = getSize();
}

void DataBuffer::setIntValue(int v, size_t idx)
{
    if(ds_.major_type != DataType::kInt){
        SPDLOG_ERROR("Data buffer is not interger type!");
        return;
    }
    if(idx >= value_size_){
        SPDLOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
        return;
    }
    int* pvalue = reinterpret_cast<int*>(fundamental_bytes_.data());
    *(pvalue + idx) = v;
}

void DataBuffer::setIntValue(const int* vp, size_t size)
{
    if(ds_.major_type != DataType::kInt){
        SPDLOG_ERROR("Data buffer is not interger type!");
        return;
    }
    if(vp == nullptr || size == 0){
        SPDLOG_ERROR("Invalid param 'vp' or 'size'!");
        return;
    }
    auto safe_size = std::min<size_t>(size, value_size_);    
    memcpy(fundamental_bytes_.data(), vp, safe_size * sizeof(int));
}

std::optional<int> DataBuffer::getIntValue(size_t idx) const
{
    if(ds_.major_type != DataType::kInt){
        SPDLOG_ERROR("Data buffer is not integer type!");
        return std::nullopt;
    }
    if(idx >= value_size_){
        SPDLOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
        return std::nullopt;
    }
    const int* pvalue = reinterpret_cast<const int*>(fundamental_bytes_.data());
    assert(pvalue);
    return std::optional<int>(*(pvalue + idx));
}

int* DataBuffer::getIntPointer()
{
    return const_cast<int*>(const_cast<const DataBuffer*>(this)->getIntPointer());
}

const int *DataBuffer::getIntPointer() const
{
    if(ds_.major_type != DataType::kInt){
        SPDLOG_ERROR("Data buffer is not integer type!");
        return nullptr;
    }
    if(fundamental_bytes_.empty())
        return nullptr;
    return reinterpret_cast<const int*>(fundamental_bytes_.data());
}

void DataBuffer::setFloatValue(float v, size_t idx)
{
    if(ds_.major_type != DataType::kFloat){
        SPDLOG_ERROR("Data buffer is not float type!");
        return;
    }
    if(idx >= value_size_){
        SPDLOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
        return;
    }
    float* pvalue = reinterpret_cast<float*>(fundamental_bytes_.data());
    *(pvalue + idx) = v;
}

void DataBuffer::setFloatValue(const float* vp, size_t size)
{
    if(ds_.major_type != DataType::kFloat){
        SPDLOG_ERROR("Data buffer is not float type!");
        return;
    }
    if(vp == nullptr || size == 0){
        SPDLOG_ERROR("Invalid param 'vp' or 'size'!");
        return;
    }
    auto safe_size = std::min<size_t>(size, value_size_);    
    memcpy(fundamental_bytes_.data(), vp, safe_size * sizeof(float));
}

std::optional<float> DataBuffer::getFloatValue(size_t idx) const
{
    if(ds_.major_type != DataType::kFloat){
        SPDLOG_ERROR("Data buffer is not float type!");
        return std::nullopt;
    }
    if(idx >= value_size_){
        SPDLOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
        return std::nullopt;
    }
    const float* pvalue = reinterpret_cast<const float*>(fundamental_bytes_.data());
    assert(pvalue);
    return std::optional<float>(*(pvalue + idx));
}

float *DataBuffer::getFloatPointer()
{
    return const_cast<float*>(const_cast<const DataBuffer*>(this)->getFloatPointer());
}

const float *DataBuffer::getFloatPointer() const
{
    if(ds_.major_type != DataType::kFloat){
        SPDLOG_ERROR("Data buffer is not float type!");
        return nullptr;
    }
    if(fundamental_bytes_.empty())
        return nullptr;
    return reinterpret_cast<const float*>(fundamental_bytes_.data());
}

void DataBuffer::setStringValue(const char *s, size_t idx)
{
}

void DataBuffer::setStringValue(const AgvMultiString &strs)
{
}

const char *DataBuffer::getStringValue(size_t idx) const
{
    return nullptr;
}

AgvMultiString DataBuffer::getAllStringValues() const
{
    return AgvMultiString();
}

void DataBuffer::setBytesValue(const agv_byte* source, size_t byte_size, size_t idx)
{
    if(ds_.major_type != DataType::kBytes){
        SPDLOG_ERROR("Data buffer is not 'bytes' type!");
        return;
    }
    if(idx >= value_size_){
        SPDLOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
        return;
    }
    if(source == nullptr || byte_size == 0){
        bytes_table_[idx].clear();
        return;
    }
    auto& dst_bytes = bytes_table_[idx];
    memcpy(dst_bytes.data(), source, byte_size);
}

std::optional<AgvBytes> DataBuffer::getBytesValue(size_t idx) const
{
    if(ds_.major_type != DataType::kBytes){
        SPDLOG_ERROR("Data buffer is not 'bytes' type!");
        return std::nullopt;
    }
    if(idx >= value_size_){
        SPDLOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
        return std::nullopt;
    }
    return std::optional<AgvBytes>(bytes_table_[idx]);
}

const AgvMultiBytes& DataBuffer::getAllBytesValues() const
{
    return bytes_table_;
}

AGV_NAMESPACE_END
