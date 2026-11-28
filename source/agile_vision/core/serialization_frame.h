/*
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *
 *   File: serialization_frame.h
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

#ifndef __agile_vision_core_serialization_frame_h__
#define __agile_vision_core_serialization_frame_h__

#include <cstdint>
#include <string>
#include "agile_vision/basic/basic_typedef.h"

namespace agv::core::detail {

    // 各层帧类型标识，ASCII 小端排列：AGVS/AGVR/AGVP/AGVT。
    constexpr uint32_t kSolutionFrameMagic = 0x53475641;
    constexpr uint32_t kProcedureFrameMagic = 0x52564741;
    constexpr uint32_t kProcessFrameMagic = 0x50564741;
    constexpr uint32_t kToolFrameMagic = 0x54564741;
    // 工具帧尾部的可选子过程段标记（ASCII 小端 "PCGM"），旧帧无此段仍可解析。
    constexpr uint32_t kChildProcessSegmentMagic = 0x4D474350;
    constexpr uint32_t kTopologyVersion = 1;
    constexpr int kMaxTreeDepth = 16;
    constexpr int kMaxTreeNodes = 256;

    /// @brief 递归加载上下文：限制子过程树深度与节点总数，防止畸形帧导致栈溢出或超大分配。
    struct TopologyContext
    {
        int depth = 0;
        int node_count = 0;
    };
    constexpr size_t kMaxFrameBytes = 256u * 1024u * 1024u;
    constexpr size_t kMaxFieldBytes = 64u * 1024u * 1024u;

    // 以下编码统一使用小端，避免宿主端序影响方案文件。
    inline void writeU32(AgvBytes& out, uint32_t v)
    {
        out.push_back(static_cast<char>(v & 0xFFu));
        out.push_back(static_cast<char>((v >> 8) & 0xFFu));
        out.push_back(static_cast<char>((v >> 16) & 0xFFu));
        out.push_back(static_cast<char>((v >> 24) & 0xFFu));
    }

    inline bool readU32(ConsAgvBytePtr& cur, size_t& left, uint32_t& v)
    {
        if(left < 4){
            return false;
        }
        const auto* p = reinterpret_cast<const unsigned char*>(cur);
        v = static_cast<uint32_t>(p[0]) |
            (static_cast<uint32_t>(p[1]) << 8) |
            (static_cast<uint32_t>(p[2]) << 16) |
            (static_cast<uint32_t>(p[3]) << 24);
        cur += 4;
        left -= 4;
        return true;
    }

    inline void writeU8(AgvBytes& out, uint8_t v)
    {
        out.push_back(static_cast<char>(v));
    }

    inline bool readU8(ConsAgvBytePtr& cur, size_t& left, uint8_t& v)
    {
        if(left < 1){
            return false;
        }
        v = static_cast<uint8_t>(*cur);
        ++cur;
        --left;
        return true;
    }

    inline void writeBytes(AgvBytes& out, const void* data, size_t size)
    {
        writeU32(out, static_cast<uint32_t>(size));
        const auto* p = static_cast<const char*>(data);
        out.insert(out.end(), p, p + size);
    }

    inline bool readBytes(ConsAgvBytePtr& cur, size_t& left, const char*& data, size_t& size)
    {
        uint32_t len = 0;
        if(!readU32(cur, left, len) || len > kMaxFieldBytes || len > left){
            return false;
        }
        data = cur;
        size = len;
        cur += len;
        left -= len;
        return true;
    }

    inline void writeString(AgvBytes& out, const std::string& s)
    {
        writeBytes(out, s.data(), s.size());
    }

    inline bool readString(ConsAgvBytePtr& cur, size_t& left, std::string& s)
    {
        const char* data = nullptr;
        size_t len = 0;
        if(!readBytes(cur, left, data, len)){
            return false;
        }
        s.assign(data, len);
        return true;
    }

    inline void writeFrameHeader(AgvBytes& out, uint32_t magic, uint32_t payload_size)
    {
        writeU32(out, magic);
        writeU32(out, kTopologyVersion);
        writeU32(out, payload_size);
    }

    inline bool readFrameHeader(ConsAgvBytePtr& cur, size_t& left, uint32_t expected_magic, uint32_t& payload_size)
    {
        uint32_t magic = 0;
        uint32_t version = 0;
        if(!readU32(cur, left, magic) || !readU32(cur, left, version) || !readU32(cur, left, payload_size)){
            return false;
        }
        if(magic != expected_magic || version != kTopologyVersion){
            return false;
        }
        return payload_size <= left && payload_size <= kMaxFrameBytes;
    }

}

#endif
