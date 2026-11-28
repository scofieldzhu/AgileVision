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

#include <algorithm>
#include <cstring>
#include "data_buffer.h"
#include "agile_vision/geometry/vec_proxy.hpp"
#include "agile_vision/basic/log_service.h"
#include "agile_vision/basic/portable_stream.h"

namespace agv::core {

    namespace{
        using VecByteProxy = agv::VecProxy<agile_vision_byte>;
        using VecVBPProxy = agv::VecProxy<VecByteProxy>;
    }

    DataBuffer::DataBuffer(const DataSpec& spec)
        :ds_(spec)
    {
        if(isFixedSize()){
            if(isFundamentalType(ds_.major_type)){
                fundamental_bytes_.resize(ds_.arry_size * getFundamentalTypeSize(ds_.major_type), 0);
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
        std::shared_lock lk(io_mutex_);
        AgvBytes payload;
        agv::detail::appendBytes(payload, ds_.serializeToBytes());

        // 基础类型数组按小端逐元素编码；图像仅写入元数据，不写入进程内指针。
        AgvBytes fundamental_payload;
        if(isFundamentalType(ds_.major_type)){
            const auto item_size = getFundamentalTypeSize(ds_.major_type);
            const size_t count = item_size == 0 ? 0 : fundamental_bytes_.size() / item_size;
            for(size_t i = 0; i < count; ++i){
                const auto* raw = fundamental_bytes_.data() + i * item_size;
                if(ds_.major_type == DataType::kInt){
                    int v = 0;
                    std::memcpy(&v, raw, sizeof(v));
                    agv::detail::writeScalar(fundamental_payload, v);
                }else if(ds_.major_type == DataType::kFloat)
                {
                    float v = 0.0f;
                    std::memcpy(&v, raw, sizeof(v));
                    agv::detail::writeScalar(fundamental_payload, v);
                }else
                {
                    ImageData img{};
                    std::memcpy(&img, raw, sizeof(img));
                    agv::detail::writeU8(fundamental_payload, img.type);
                    agv::detail::writeU8(fundamental_payload, img.channel);
                    agv::detail::writeU8(fundamental_payload, img.bits);
                    agv::detail::writeU32(fundamental_payload, img.width);
                    agv::detail::writeU32(fundamental_payload, img.height);
                    agv::detail::writeU32(fundamental_payload, img.size);
                }
            }
        }
        agv::detail::writeU32(payload, static_cast<uint32_t>(fundamental_payload.size()));
        agv::detail::appendBytes(payload, fundamental_payload);

        VecVBPProxy vvpp;
        if(!bytes_table_.empty()){
            auto& values = vvpp.mutableData();
            values.resize(bytes_table_.size());
            for(size_t i = 0; i < bytes_table_.size(); ++i){
                values[i].mutableData() = bytes_table_[i];
            }
        }
        agv::detail::appendBytes(payload, vvpp.serializeToBytes());
        agv::detail::writeU32(payload, static_cast<uint32_t>(value_size_));

        AgvBytes framed;
        agv::detail::writeFramedBytes(framed, agv::detail::kDataBufferMagic, payload);
        return framed;
    }

    size_t DataBuffer::loadBytes(ConsAgvBytePtr buffer, size_t size)
    {
        std::unique_lock lk(io_mutex_);
        ConsAgvBytePtr payload = nullptr;
        size_t payload_size = 0;
        if(!agv::detail::readFramedBytes(buffer, size, agv::detail::kDataBufferMagic, payload, payload_size)){
            return 0;
        }
        ConsAgvBytePtr cur = payload;
        size_t left = payload_size;
        auto finish_bytes = ds_.loadBytes(cur, left);
        if(finish_bytes == 0 || finish_bytes > left){
            return 0;
        }
        cur += finish_bytes;
        left -= finish_bytes;

        uint32_t fundamental_size = 0;
        if(!agv::detail::readU32(cur, left, fundamental_size) || fundamental_size > left){
            return 0;
        }
        ConsAgvBytePtr fund_cur = cur;
        size_t fund_left = fundamental_size;
        size_t encoded_item_size = 0;
        if(ds_.major_type == DataType::kInt || ds_.major_type == DataType::kFloat){
            encoded_item_size = 4;
        }else if(ds_.major_type == DataType::kImage)
        {
            encoded_item_size = 15;
        }
        if(encoded_item_size != 0 && fundamental_size % encoded_item_size != 0){
            return 0;
        }
        const size_t count = encoded_item_size == 0 ? 0 : fundamental_size / encoded_item_size;
        fundamental_bytes_.clear();
        if(ds_.major_type == DataType::kInt){
            fundamental_bytes_.resize(count * sizeof(int), 0);
            for(size_t i = 0; i < count; ++i){
                int v = 0;
                if(!agv::detail::readScalar(fund_cur, fund_left, v)){
                    return 0;
                }
                std::memcpy(fundamental_bytes_.data() + i * sizeof(v), &v, sizeof(v));
            }
        }else if(ds_.major_type == DataType::kFloat)
        {
            fundamental_bytes_.resize(count * sizeof(float), 0);
            for(size_t i = 0; i < count; ++i){
                float v = 0.0f;
                if(!agv::detail::readScalar(fund_cur, fund_left, v)){
                    return 0;
                }
                std::memcpy(fundamental_bytes_.data() + i * sizeof(v), &v, sizeof(v));
            }
        }else if(ds_.major_type == DataType::kImage)
        {
            fundamental_bytes_.resize(count * sizeof(ImageData), 0);
            for(size_t i = 0; i < count; ++i){
                uint8_t type = 0;
                uint8_t channel = 0;
                uint8_t bits = 0;
                uint32_t width = 0;
                uint32_t height = 0;
                uint32_t image_size = 0;
                if(!agv::detail::readU8(fund_cur, fund_left, type)){
                    return 0;
                }
                if(!agv::detail::readU8(fund_cur, fund_left, channel)){
                    return 0;
                }
                if(!agv::detail::readU8(fund_cur, fund_left, bits)){
                    return 0;
                }
                if(!agv::detail::readU32(fund_cur, fund_left, width)){
                    return 0;
                }
                if(!agv::detail::readU32(fund_cur, fund_left, height)){
                    return 0;
                }
                if(!agv::detail::readU32(fund_cur, fund_left, image_size)){
                    return 0;
                }
                ImageData img{};
                img.type = type;
                img.channel = channel;
                img.bits = bits;
                img.width = width;
                img.height = height;
                img.size = image_size;
                std::memcpy(fundamental_bytes_.data() + i * sizeof(img), &img, sizeof(img));
            }
        }
        if(fund_left != 0){
            return 0;
        }
        cur += fundamental_size;
        left -= fundamental_size;

        VecVBPProxy vvpp;
        finish_bytes = vvpp.loadBytes(cur, left);
        if(finish_bytes == 0 || finish_bytes > left){
            return 0;
        }
        bytes_table_.resize(vvpp.data().size());
        for(size_t i = 0; i < bytes_table_.size(); ++i){
            bytes_table_[i] = vvpp.data()[i].data();
        }
        cur += finish_bytes;
        left -= finish_bytes;

        uint32_t value_size = 0;
        if(!agv::detail::readU32(cur, left, value_size)){
            AGV_LOG_ERROR("Serialized data buffer lacks value size field!");
            return 0;
        }
        if(left != 0){
            return 0;
        }
        // 声明值不得大于实际装载容量，避免后续按此值读写时越界。
        value_size_ = std::min<size_t>(value_size, getSize());
        version_.fetch_add(1, std::memory_order_relaxed);
        return agv::detail::kPortableHeaderSize + payload_size;
    }

    DataSpec DataBuffer::dataSpec() const
    {
        std::shared_lock lk(io_mutex_);
        return ds_;
    }

    agile_vision_time_t DataBuffer::lastModifyTime() const
    {
        std::shared_lock lk(io_mutex_);
        return last_modify_time_;
    }

    DataBufferSnapshot DataBuffer::snapshot() const
    {
        return DataBufferSnapshot(*this);
    }

    size_t DataBuffer::valueSize() const
    {
        std::shared_lock lk(io_mutex_);
        return value_size_;
    }

    size_t DataBuffer::getSize() const
    {
        return isFundamentalType(ds_.major_type) ? fundamental_bytes_.size() : bytes_table_.size();
    }

    bool DataBuffer::resize(size_t number)
    {
        std::unique_lock lk(io_mutex_);
        if(isFixedSize()){
            return false;
        }
        value_size_ = number;
        if(isFundamentalType(ds_.major_type)){
            fundamental_bytes_.resize(value_size_ * getFundamentalTypeSize(ds_.major_type), 0);
        }else{
            bytes_table_.resize(value_size_);
        }
        version_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    void DataBuffer::erase(size_t idx)
    {
        std::unique_lock lk(io_mutex_);
        if(isFixedSize()){
            AGV_LOG_ERROR("Fixed data buffer cannot execute erase operation!");
            return;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
            return;
        }
        if(isFundamentalType(ds_.major_type)){
            const size_t type_size = getFundamentalTypeSize(ds_.major_type);
            auto start_iter = fundamental_bytes_.begin() + type_size * idx;
            auto last_iter = start_iter + type_size;
            fundamental_bytes_.erase(start_iter, last_iter);
        }else{
            bytes_table_.erase(bytes_table_.begin() + idx);
        }
        value_size_ = getSize();
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    void DataBuffer::setIntValue(int v, size_t idx)
    {
        std::unique_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kInt){
            AGV_LOG_ERROR("Data buffer is not interger type!");
            return;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
            return;
        }
        int* pvalue = reinterpret_cast<int*>(fundamental_bytes_.data());
        *(pvalue + idx) = v;
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    void DataBuffer::setIntValue(const int* vp, size_t size)
    {
        std::unique_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kInt){
            AGV_LOG_ERROR("Data buffer is not interger type!");
            return;
        }
        if(vp == nullptr || size == 0){
            AGV_LOG_ERROR("Invalid param 'vp' or 'size'!");
            return;
        }
        auto safe_size = std::min<size_t>(size, value_size_);
        memcpy(fundamental_bytes_.data(), vp, safe_size * sizeof(int));
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    std::optional<int> DataBuffer::getIntValue(size_t idx) const
    {
        std::shared_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kInt){
            AGV_LOG_ERROR("Data buffer is not integer type!");
            return std::nullopt;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
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
            AGV_LOG_ERROR("Data buffer is not integer type!");
            return nullptr;
        }
        if(fundamental_bytes_.empty()){
            return nullptr;
        }
        return reinterpret_cast<const int*>(fundamental_bytes_.data());
    }

    void DataBuffer::setFloatValue(float v, size_t idx)
    {
        std::unique_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kFloat){
            AGV_LOG_ERROR("Data buffer is not float type!");
            return;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
            return;
        }
        float* pvalue = reinterpret_cast<float*>(fundamental_bytes_.data());
        *(pvalue + idx) = v;
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    void DataBuffer::setFloatValue(const float* vp, size_t size)
    {
        std::unique_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kFloat){
            AGV_LOG_ERROR("Data buffer is not float type!");
            return;
        }
        if(vp == nullptr || size == 0){
            AGV_LOG_ERROR("Invalid param 'vp' or 'size'!");
            return;
        }
        auto safe_size = std::min<size_t>(size, value_size_);
        memcpy(fundamental_bytes_.data(), vp, safe_size * sizeof(float));
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    std::optional<float> DataBuffer::getFloatValue(size_t idx) const
    {
        std::shared_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kFloat){
            AGV_LOG_ERROR("Data buffer is not float type!");
            return std::nullopt;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
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
            AGV_LOG_ERROR("Data buffer is not float type!");
            return nullptr;
        }
        if(fundamental_bytes_.empty()){
            return nullptr;
        }
        return reinterpret_cast<const float*>(fundamental_bytes_.data());
    }

    void DataBuffer::setImageValue(const ImageData& v, size_t idx)
    {
        std::unique_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kImage){
            AGV_LOG_ERROR("Data buffer is not image type!");
            return;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
            return;
        }
        ImageData* pvalue = reinterpret_cast<ImageData*>(fundamental_bytes_.data());
        *(pvalue + idx) = v;
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    std::optional<ImageData> DataBuffer::getImageValue(size_t idx) const
    {
        std::shared_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kImage){
            AGV_LOG_ERROR("Data buffer is not image type!");
            return std::nullopt;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
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
            AGV_LOG_ERROR("Data buffer is not image type!");
            return nullptr;
        }
        if(fundamental_bytes_.empty()){
            return nullptr;
        }
        return reinterpret_cast<const ImageData*>(fundamental_bytes_.data());
    }

    void DataBuffer::setStringValue(const char* source, size_t idx)
    {
        std::unique_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kString){
            AGV_LOG_ERROR("Data buffer is not 'string' type!");
            return;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
            return;
        }
        if(source == nullptr){
            bytes_table_[idx].clear();
            version_.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        auto& dst_bytes = bytes_table_[idx];
        dst_bytes.resize(strlen(source) + 1);
        memcpy(dst_bytes.data(), source, strlen(source));
        dst_bytes[strlen(source)] = '\0';
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    void DataBuffer::setStringValue(const AgvMultiString& strs)
    {
        std::unique_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kString){
            AGV_LOG_ERROR("Data buffer is not 'string' type!");
            return;
        }
        if(strs.empty()){
            bytes_table_.clear();
            version_.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        const auto safe_size = std::min<size_t>(strs.size(), value_size_);
        if(bytes_table_.size() < safe_size){
            bytes_table_.resize(safe_size);
        }
        std::transform(strs.begin(), strs.begin() + safe_size, bytes_table_.begin(), [](const auto& s){
            AgvBytes bytes(s.size() + 1, 0);
            memcpy(bytes.data(), s.c_str(), s.size());
            bytes[s.size()] = '\0';
            return bytes;
        });
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    const char* DataBuffer::getStringValue(size_t idx) const
    {
        if(ds_.major_type != DataType::kString){
            AGV_LOG_ERROR("Data buffer is not 'string' type!");
            return nullptr;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
            return nullptr;
        }
        return (const char*)bytes_table_[idx].data();
    }

    AgvMultiString DataBuffer::getAllStringValues() const
    {
        std::shared_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kString){
            AGV_LOG_ERROR("Data buffer is not 'string' type!");
            return {};
        }
        AgvMultiString strs;
        std::for_each(bytes_table_.begin(), bytes_table_.end(), [&strs](const auto& it){
            strs.push_back((const char*)it.data());
        });
        return strs;
    }

    void DataBuffer::setBytesValue(const agile_vision_byte* source, size_t byte_size, size_t idx)
    {
        std::unique_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kBytes){
            AGV_LOG_ERROR("Data buffer is not 'bytes' type!");
            return;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
            return;
        }
        if(source == nullptr || byte_size == 0){
            bytes_table_[idx].clear();
            version_.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        auto& dst_bytes = bytes_table_[idx];
        dst_bytes.resize(byte_size);
        memcpy(dst_bytes.data(), source, byte_size);
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    std::optional<AgvBytes> DataBuffer::getBytesValue(size_t idx) const
    {
        std::shared_lock lk(io_mutex_);
        if(ds_.major_type != DataType::kBytes){
            AGV_LOG_ERROR("Data buffer is not 'bytes' type!");
            return std::nullopt;
        }
        if(idx >= value_size_){
            AGV_LOG_ERROR("Index:{} out of range! size:{}", idx, value_size_);
            return std::nullopt;
        }
        return std::optional<AgvBytes>(bytes_table_[idx]);
    }

    const AgvMultiBytes& DataBuffer::getAllBytesValues() const
    {
        return bytes_table_;
    }

    void DataBuffer::updateModifyTime(agile_vision_time_t t)
    {
        std::unique_lock lk(io_mutex_);
        last_modify_time_ = t;
        version_.fetch_add(1, std::memory_order_relaxed);
    }

    namespace
    {
        AgvBytes::const_iterator findTerminator(const AgvBytes& bytes)
        {
            return std::find(bytes.begin(), bytes.end(), static_cast<agile_vision_byte>(0));
        }
    }

    DataBufferSnapshot::DataBufferSnapshot(const DataBuffer& src)
    {
        std::shared_lock lk(src.io_mutex_);
        ds_ = src.ds_;
        fundamental_bytes_ = src.fundamental_bytes_;
        bytes_table_ = src.bytes_table_;
        value_size_ = src.value_size_;
        version_ = src.version_.load(std::memory_order_relaxed);
    }

    std::optional<int> DataBufferSnapshot::getIntValue(size_t idx) const
    {
        const size_t element_size = sizeof(int);
        const size_t required_bytes = (idx + 1) * element_size;
        if(ds_.major_type != DataType::kInt || idx >= value_size_ || required_bytes > fundamental_bytes_.size()){
            return std::nullopt;
        }
        const auto* pvalue = reinterpret_cast<const int*>(fundamental_bytes_.data());
        return std::optional<int>(*(pvalue + idx));
    }

    std::optional<float> DataBufferSnapshot::getFloatValue(size_t idx) const
    {
        const size_t element_size = sizeof(float);
        const size_t required_bytes = (idx + 1) * element_size;
        if(ds_.major_type != DataType::kFloat || idx >= value_size_ || required_bytes > fundamental_bytes_.size()){
            return std::nullopt;
        }
        const auto* pvalue = reinterpret_cast<const float*>(fundamental_bytes_.data());
        return std::optional<float>(*(pvalue + idx));
    }

    std::optional<ImageData> DataBufferSnapshot::getImageValue(size_t idx) const
    {
        const size_t element_size = sizeof(ImageData);
        const size_t required_bytes = (idx + 1) * element_size;
        if(ds_.major_type != DataType::kImage || idx >= value_size_ || required_bytes > fundamental_bytes_.size()){
            return std::nullopt;
        }
        const auto* pvalue = reinterpret_cast<const ImageData*>(fundamental_bytes_.data());
        return std::optional<ImageData>(*(pvalue + idx));
    }

    std::optional<std::string> DataBufferSnapshot::getStringValue(size_t idx) const
    {
        if(ds_.major_type != DataType::kString || idx >= value_size_ || idx >= bytes_table_.size()){
            return std::nullopt;
        }
        const auto& bytes = bytes_table_[idx];
        const auto terminator = findTerminator(bytes);
        return std::string(reinterpret_cast<const char*>(bytes.data()), static_cast<size_t>(terminator - bytes.begin()));
    }

    std::optional<AgvBytes> DataBufferSnapshot::getBytesValue(size_t idx) const
    {
        if(ds_.major_type != DataType::kBytes || idx >= value_size_ || idx >= bytes_table_.size()){
            return std::nullopt;
        }
        return std::optional<AgvBytes>(bytes_table_[idx]);
    }

}
