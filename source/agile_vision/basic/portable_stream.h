/*
 *   AgileVision is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *
 *   File: portable_stream.h
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

#ifndef __agile_vision_basic_portable_stream_h__
#define __agile_vision_basic_portable_stream_h__

#include <bit>
#include <cstdint>
#include <type_traits>
#include "agile_vision/basic/basic_typedef.h"

namespace agv::detail {

    // 所有 Proxy 字节流统一使用小端编码，避免宿主端序影响持久化文件。
    constexpr uint32_t kPortableStreamVersion = 1;
    constexpr size_t kPortableHeaderSize = 12;

    // 帧魔数按 ASCII 小端排列，便于直接查看字节流前缀。
    constexpr uint32_t kStringProxyMagic = 0x50525453;     // "STRP"
    constexpr uint32_t kArrayXMagic = 0x58525241;          // "ARRX"
    constexpr uint32_t kElementProxyMagic = 0x504D4C45;    // "ELMP"
    constexpr uint32_t kVecProxyMagic = 0x50434556;        // "VECP"
    constexpr uint32_t kDictProxyMagic = 0x50544344;       // "DCTP"
    constexpr uint32_t kRectMagic = 0x54434552;            // "RECT"
    constexpr uint32_t kDataBufferMagic = 0x42544144;      // "DATB"
    constexpr uint32_t kGlobalVariableMagic = 0x56424C47;  // "GLBV"

    inline void writeU8(ByteVec& out, uint8_t v)
    {
        out.push_back(static_cast<Byte>(v));
    }

    inline bool readU8(ConsBytePtr& cur, size_t& left, uint8_t& v)
    {
        if(left < 1){
            return false;
        }
        v = static_cast<uint8_t>(static_cast<unsigned char>(*cur));
        ++cur;
        --left;
        return true;
    }

    inline void writeU16(ByteVec& out, uint16_t v)
    {
        for(size_t i = 0; i < 2; ++i){
            out.push_back(static_cast<Byte>((v >> (8 * i)) & 0xFFu));
        }
    }

    inline bool readU16(ConsBytePtr& cur, size_t& left, uint16_t& v)
    {
        if(left < 2){
            return false;
        }
        uint16_t raw = 0;
        for(size_t i = 0; i < 2; ++i){
            const auto byte = static_cast<uint16_t>(static_cast<unsigned char>(cur[i]));
            raw |= static_cast<uint16_t>(byte << (8 * i));
        }
        v = raw;
        cur += 2;
        left -= 2;
        return true;
    }

    inline void writeU32(ByteVec& out, uint32_t v)
    {
        for(size_t i = 0; i < 4; ++i){
            out.push_back(static_cast<Byte>((v >> (8 * i)) & 0xFFu));
        }
    }

    inline bool readU32(ConsBytePtr& cur, size_t& left, uint32_t& v)
    {
        if(left < 4){
            return false;
        }
        uint32_t raw = 0;
        for(size_t i = 0; i < 4; ++i){
            const auto byte = static_cast<uint32_t>(static_cast<unsigned char>(cur[i]));
            raw |= byte << (8 * i);
        }
        v = raw;
        cur += 4;
        left -= 4;
        return true;
    }

    inline void writeU64(ByteVec& out, uint64_t v)
    {
        for(size_t i = 0; i < 8; ++i){
            out.push_back(static_cast<Byte>((v >> (8 * i)) & 0xFFu));
        }
    }

    inline bool readU64(ConsBytePtr& cur, size_t& left, uint64_t& v)
    {
        if(left < 8){
            return false;
        }
        uint64_t raw = 0;
        for(size_t i = 0; i < 8; ++i){
            const auto byte = static_cast<uint64_t>(static_cast<unsigned char>(cur[i]));
            raw |= byte << (8 * i);
        }
        v = raw;
        cur += 8;
        left -= 8;
        return true;
    }

    // 标量统一按固定字节数小端写出；浮点通过 bit_cast 保持位模式一致。
    template <typename T>
    void writeScalar(ByteVec& out, T v)
    {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
            "Unsupported scalar size");
        if constexpr(std::is_same_v<T, bool>){
            writeU8(out, v ? 1u : 0u);
        }else if constexpr(std::is_integral_v<T>)
        {
            if constexpr(sizeof(T) == 1){
                writeU8(out, static_cast<uint8_t>(v));
            }else if constexpr(sizeof(T) == 2)
            {
                writeU16(out, static_cast<uint16_t>(v));
            }else if constexpr(sizeof(T) == 4)
            {
                writeU32(out, static_cast<uint32_t>(v));
            }else
            {
                writeU64(out, static_cast<uint64_t>(v));
            }
        }else
        {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported floating point size");
            if constexpr(sizeof(T) == 4){
                writeU32(out, std::bit_cast<uint32_t>(v));
            }else
            {
                writeU64(out, std::bit_cast<uint64_t>(v));
            }
        }
    }

    template <typename T>
    bool readScalar(ConsBytePtr& cur, size_t& left, T& v)
    {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
            "Unsupported scalar size");
        if(left < sizeof(T)){
            return false;
        }
        if constexpr(std::is_same_v<T, bool>){
            v = static_cast<unsigned char>(*cur) != 0u;
            ++cur;
            --left;
            return true;
        }else if constexpr(std::is_integral_v<T>)
        {
            using U = std::make_unsigned_t<T>;
            U raw = 0;
            for(size_t i = 0; i < sizeof(T); ++i){
                const auto byte = static_cast<U>(static_cast<unsigned char>(cur[i]));
                raw |= static_cast<U>(byte << (8 * i));
            }
            v = static_cast<T>(raw);
        }else
        {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported floating point size");
            if constexpr(sizeof(T) == 4){
                uint32_t raw = 0;
                for(size_t i = 0; i < 4; ++i){
                    const auto byte = static_cast<uint32_t>(static_cast<unsigned char>(cur[i]));
                    raw |= byte << (8 * i);
                }
                v = std::bit_cast<T>(raw);
            }else
            {
                uint64_t raw = 0;
                for(size_t i = 0; i < 8; ++i){
                    const auto byte = static_cast<uint64_t>(static_cast<unsigned char>(cur[i]));
                    raw |= byte << (8 * i);
                }
                v = std::bit_cast<T>(raw);
            }
        }
        cur += sizeof(T);
        left -= sizeof(T);
        return true;
    }

    inline void appendBytes(ByteVec& out, const ByteVec& in)
    {
        out.insert(out.end(), in.begin(), in.end());
    }

    inline void appendBytes(ByteVec& out, ConsBytePtr data, size_t size)
    {
        out.insert(out.end(), data, data + size);
    }

    // 统一帧头：[magic:u32][version:u32][payload_size:u32]，字段均为小端。
    inline void writeFramedBytes(ByteVec& out, uint32_t magic, const ByteVec& payload)
    {
        writeU32(out, magic);
        writeU32(out, kPortableStreamVersion);
        writeU32(out, static_cast<uint32_t>(payload.size()));
        appendBytes(out, payload);
    }

    inline bool readFramedBytes(ConsBytePtr buffer, size_t size, uint32_t magic, ConsBytePtr& payload, size_t& payload_size)
    {
        if(buffer == nullptr || size < kPortableHeaderSize){
            return false;
        }
        ConsBytePtr cur = buffer;
        size_t left = size;
        uint32_t frame_magic = 0;
        uint32_t version = 0;
        uint32_t length = 0;
        if(!readU32(cur, left, frame_magic) || !readU32(cur, left, version) || !readU32(cur, left, length)){
            return false;
        }
        if(frame_magic != magic || version != kPortableStreamVersion || length > left){
            return false;
        }
        payload = cur;
        payload_size = length;
        return true;
    }

}

#endif
