/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: data_buffer.cpp  
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

#include "data_buffer.h"
#include "ratel/geometry/vec_proxy.hpp"
#include "spdlog/spdlog.h"

AGV_NAMESPACE_BEGIN

namespace{
    using VecByteProxy = ratel::VecProxy<agv_byte>;
    using VecVBPProxy = ratel::VecProxy<VecByteProxy>;
}

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

AgvBytes DataBuffer::serializeToBytes() const
{
    size_t total_bytes = 0;
    
    auto ds_bv = ds_.serializeToBytes();
    total_bytes += ds_bv.size();

    VecByteProxy vbp;
    if(!fundamental_bytes_.empty()){
        auto& vbp_m_data = vbp.mutableData();
        vbp_m_data.resize(fundamental_bytes_.size());
        memcpy(vbp_m_data.data(), fundamental_bytes_.data(), fundamental_bytes_.size());
    }
    auto fdb_bv = vbp.serializeToBytes();
    total_bytes += fdb_bv.size();

    VecVBPProxy vvpp;
    if(!bytes_table_.empty()){
        auto& vvpp_m_data = vvpp.mutableData();
        vvpp_m_data.resize(bytes_table_.size());
        for(unsigned int i = 0; i < bytes_table_.size(); ++i)
            vvpp_m_data[i].mutableData() = bytes_table_[i];
    }
    auto vvpp_bv = vvpp.serializeToBytes();
    total_bytes += vvpp_bv.size();

    AgvBytes bv(total_bytes + ratel::kUIntSize, 0);
    auto cur_data = bv.data();
    memcpy(cur_data, ds_bv.data(), ds_bv.size());
    cur_data += ds_bv.size();
    memcpy(cur_data, fdb_bv.data(), fdb_bv.size());
    cur_data += fdb_bv.size();
    memcpy(cur_data, vvpp_bv.data(), vvpp_bv.size());
    cur_data += vvpp_bv.size();
    auto value_size = (unsigned int)value_size_;
    memcpy(cur_data, &value_size, ratel::kUIntSize);
    return bv;
}

size_t DataBuffer::loadBytes(ConsAgvBytePtr buffer, size_t size)
{
    auto cur_data = buffer;
    auto left_size = size;
    auto finish_bytes = ds_.loadBytes(cur_data, left_size);
    if(finish_bytes == 0)
        return 0;
    cur_data += finish_bytes;
    left_size -= finish_bytes;
    VecByteProxy vbp;
    finish_bytes = vbp.loadBytes(cur_data, left_size);
    if(finish_bytes == 0)
        return 0;
    fundamental_bytes_ = vbp.data();

    cur_data += finish_bytes;
    left_size -= finish_bytes;
    VecVBPProxy vvpp;
    finish_bytes = vvpp.loadBytes(cur_data, left_size);
    if(finish_bytes == 0)
        return 0;
    bytes_table_.resize(vvpp.data().size());
    for(unsigned int i = 0; i < bytes_table_.size(); ++i)
        bytes_table_[i] = vvpp.data().at(i).data();

    unsigned int value_size = 0;
    memcpy(&value_size, cur_data, ratel::kUIntSize);
    value_size_ = value_size;
    cur_data += ratel::kUIntSize;
    left_size -= ratel::kUIntSize;
    return size - left_size;
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
        spdlog::error("Fixed data buffer cannot execute erase operation!");
        return;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
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
        spdlog::error("Data buffer is not interger type!");
        return;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
        return;
    }
    int* pvalue = reinterpret_cast<int*>(fundamental_bytes_.data());
    *(pvalue + idx) = v;
}

void DataBuffer::setIntValue(const int* vp, size_t size)
{
    if(ds_.major_type != DataType::kInt){
        spdlog::error("Data buffer is not interger type!");
        return;
    }
    if(vp == nullptr || size == 0){
        spdlog::error("Invalid param 'vp' or 'size'!");
        return;
    }
    auto safe_size = std::min<size_t>(size, value_size_);    
    memcpy(fundamental_bytes_.data(), vp, safe_size * sizeof(int));
}

std::optional<int> DataBuffer::getIntValue(size_t idx) const
{
    if(ds_.major_type != DataType::kInt){
        spdlog::error("Data buffer is not integer type!");
        return std::nullopt;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
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
        spdlog::error("Data buffer is not integer type!");
        return nullptr;
    }
    if(fundamental_bytes_.empty())
        return nullptr;
    return reinterpret_cast<const int*>(fundamental_bytes_.data());
}

void DataBuffer::setFloatValue(float v, size_t idx)
{
    if(ds_.major_type != DataType::kFloat){
        spdlog::error("Data buffer is not float type!");
        return;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
        return;
    }
    float* pvalue = reinterpret_cast<float*>(fundamental_bytes_.data());
    *(pvalue + idx) = v;
}

void DataBuffer::setFloatValue(const float* vp, size_t size)
{
    if(ds_.major_type != DataType::kFloat){
        spdlog::error("Data buffer is not float type!");
        return;
    }
    if(vp == nullptr || size == 0){
        spdlog::error("Invalid param 'vp' or 'size'!");
        return;
    }
    auto safe_size = std::min<size_t>(size, value_size_);    
    memcpy(fundamental_bytes_.data(), vp, safe_size * sizeof(float));
}

std::optional<float> DataBuffer::getFloatValue(size_t idx) const
{
    if(ds_.major_type != DataType::kFloat){
        spdlog::error("Data buffer is not float type!");
        return std::nullopt;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
        return std::nullopt;
    }
    const float* pvalue = reinterpret_cast<const float*>(fundamental_bytes_.data());
    assert(pvalue);
    return std::optional<float>(*(pvalue + idx));
}

float* DataBuffer::getFloatPointer()
{
    return const_cast<float*>(const_cast<const DataBuffer*>(this)->getFloatPointer());
}

const float *DataBuffer::getFloatPointer() const
{
    if(ds_.major_type != DataType::kFloat){
        spdlog::error("Data buffer is not float type!");
        return nullptr;
    }
    if(fundamental_bytes_.empty())
        return nullptr;
    return reinterpret_cast<const float*>(fundamental_bytes_.data());
}

void DataBuffer::setImageValue(const ImageData& v, size_t idx)
{
    if(ds_.major_type != DataType::kImage){
        spdlog::error("Data buffer is not image type!");
        return;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
        return;
    }
    ImageData* pvalue = reinterpret_cast<ImageData*>(fundamental_bytes_.data());
    *(pvalue + idx) = v;
}

std::optional<ImageData> DataBuffer::getImageValue(size_t idx) const
{
    if(ds_.major_type != DataType::kImage){
        spdlog::error("Data buffer is not image type!");
        return std::nullopt;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
        return std::nullopt;
    }
    const ImageData* pvalue = reinterpret_cast<const ImageData*>(fundamental_bytes_.data());
    assert(pvalue);
    return std::optional<ImageData>(*(pvalue + idx));
}

ImageData* DataBuffer::getImagePointer()
{
    return const_cast<ImageData*>(const_cast<const DataBuffer*>(this)->getImagePointer());
}

const ImageData* DataBuffer::getImagePointer() const
{
    if(ds_.major_type != DataType::kImage){
        spdlog::error("Data buffer is not image type!");
        return nullptr;
    }
    if(fundamental_bytes_.empty())
        return nullptr;
    return reinterpret_cast<const ImageData*>(fundamental_bytes_.data());
}

void DataBuffer::setStringValue(const char* source, size_t idx)
{
    if(ds_.major_type != DataType::kString){
        spdlog::error("Data buffer is not 'string' type!");
        return;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
        return;
    }
    if(source == nullptr){
        bytes_table_[idx].clear();
        return;
    }
    auto& dst_bytes = bytes_table_[idx];
    dst_bytes.resize(strlen(source) + 1);
    memcpy(dst_bytes.data(), source, strlen(source));
    dst_bytes[strlen(source)] = '\0';
}

void DataBuffer::setStringValue(const AgvMultiString& strs)
{
    if(ds_.major_type != DataType::kString){
        spdlog::error("Data buffer is not 'string' type!");
        return;
    }
    if(strs.empty()){
        bytes_table_.clear();
        return;
    }
    auto safe_size = std::min<size_t>(strs.size(), value_size_);
    std::transform(strs.begin(), strs.end(), bytes_table_.begin(), [](const auto& s){
        AgvBytes bytes(s.size() + 1, 0);
        memcpy(bytes.data(), s.c_str(), s.size());
        bytes[s.size()] = '\0';
        return bytes;
    });
}

const char* DataBuffer::getStringValue(size_t idx) const
{
    if(ds_.major_type != DataType::kString){
        spdlog::error("Data buffer is not 'string' type!");
        return nullptr;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
        return nullptr;
    }
    return (const char*)bytes_table_[idx].data();
}

AgvMultiString DataBuffer::getAllStringValues() const
{
    if(ds_.major_type != DataType::kString){
        spdlog::error("Data buffer is not 'string' type!");
        return {};
    }
    AgvMultiString strs;
    std::for_each(bytes_table_.begin(), bytes_table_.end(), [&strs](const auto& it){
        strs.push_back((const char*)it.data());
    });
    return strs;
}

void DataBuffer::setBytesValue(const agv_byte* source, size_t byte_size, size_t idx)
{
    if(ds_.major_type != DataType::kBytes){
        spdlog::error("Data buffer is not 'bytes' type!");
        return;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
        return;
    }
    if(source == nullptr || byte_size == 0){
        bytes_table_[idx].clear();
        return;
    }
    auto& dst_bytes = bytes_table_[idx];
    dst_bytes.resize(byte_size);
    memcpy(dst_bytes.data(), source, byte_size);
}

std::optional<AgvBytes> DataBuffer::getBytesValue(size_t idx) const
{
    if(ds_.major_type != DataType::kBytes){
        spdlog::error("Data buffer is not 'bytes' type!");
        return std::nullopt;
    }
    if(idx >= value_size_){
        spdlog::error("Index:{} out of range! size:{}", idx, value_size_);
        return std::nullopt;
    }
    return std::optional<AgvBytes>(bytes_table_[idx]);
}

const AgvMultiBytes& DataBuffer::getAllBytesValues() const
{
    return bytes_table_;
}

AGV_NAMESPACE_END
