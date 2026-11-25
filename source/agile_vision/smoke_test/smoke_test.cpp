/*
 *   AgileVision smoke test: self-contained, non-interactive checks for the
 *   framework's critical serialization and data-buffer invariants.
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

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include "agile_vision/geometry/line.hpp"
#include "agile_vision/geometry/circle.hpp"
#include "agile_vision/geometry/vec_proxy.hpp"
#include "agile_vision/geometry/array_x.hpp"
#include "agile_vision/core/data_spec.h"
#include "agile_vision/core/data_buffer.h"
#include "agile_vision/core/global_variable.h"

namespace {

    int g_failures = 0;

    void check(bool ok, const char* case_name)
    {
        if(!ok)
        {
            std::printf("[FAIL] %s\n", case_name);
            ++g_failures;
        } else
        {
            std::printf("[ OK ] %s\n", case_name);
        }
    }

    void testLineRoundTrip()
    {
        agv::Line2 src(agv::Point2{1, 2}, agv::Point2{3, 4});
        auto bytes = src.serializeToBytes();
        agv::Line2 dst(agv::Point2{0, 0}, agv::Point2{0, 0});
        auto used = dst.loadBytes(bytes.data(), bytes.size());
        const bool size_ok = used == bytes.size();
        const bool p1_ok = dst.p1()[0] == 1 && dst.p1()[1] == 2;
        const bool p2_ok = dst.p2()[0] == 3 && dst.p2()[1] == 4;
        check(size_ok && p1_ok && p2_ok, "Line2 serialize/load round-trip");
    }

    void testCircleRoundTrip()
    {
        agv::Circle<int> src(agv::ArrayX<int, 2>{5, 6}, 7.5f);
        auto bytes = src.serializeToBytes();
        agv::Circle<int> dst(agv::ArrayX<int, 2>{0, 0}, 0.0f);
        auto used = dst.loadBytes(bytes.data(), bytes.size());
        const bool size_ok = used == bytes.size();
        const bool center_ok = dst.center()[0] == 5 && dst.center()[1] == 6;
        const bool radius_ok = dst.radius() == 7.5f;
        check(size_ok && center_ok && radius_ok, "Circle serialize/load round-trip");
    }

    void testDataSpecRoundTrip()
    {
        agv::core::DataSpec src = agv::core::DataSpec::StaticArray(agv::DataType::kFloat, 3);
        src.subtype = 9;
        auto bytes = src.serializeToBytes();
        agv::core::DataSpec dst;
        auto used = dst.loadBytes(bytes.data(), bytes.size());
        const bool size_ok = used == bytes.size();
        const bool fields_ok = dst.major_type == agv::DataType::kFloat && dst.subtype == 9 && dst.arry_size == 3;
        check(size_ok && fields_ok, "DataSpec loadBytes writes back members");
    }

    void testDataBufferStringBound()
    {
        // 定长为 2 的字符串缓冲写入 5 个元素，必须截断而不越界。
        agv::core::DataBuffer buf(agv::core::DataSpec::StaticArray(agv::DataType::kString, 2));
        agv::AgvMultiString values = {"a", "b", "c", "d", "e"};
        buf.setStringValue(values);
        auto stored = buf.getAllStringValues();
        check(stored.size() == 2, "DataBuffer fixed-size string write is bounded");
    }

    void testMalformedVecInput()
    {
        // 只有 count 字段且声明超大元素数的畸形输入必须被拒绝（返回 0），不得超大分配。
        unsigned int huge_count = 0xFFFFFFFFu;
        std::vector<char> malformed(sizeof(huge_count));
        std::memcpy(malformed.data(), &huge_count, sizeof(huge_count));
        agv::VecProxy<int> proxy;
        auto used = proxy.loadBytes(malformed.data(), malformed.size());
        check(used == 0, "VecProxy rejects malformed oversized count");
    }

    void testGlobalVariableRoundTrip()
    {
        agv::core::GlobalVariable int_var(agv::core::GlobalVariable::DT_INT);
        int_var.setInt(20261125);
        auto int_bytes = int_var.serializeToBytes();
        agv::core::GlobalVariable int_dst(agv::core::GlobalVariable::DT_BOOLEAN);
        auto int_used = int_dst.loadBytes(int_bytes.data(), int_bytes.size());
        const bool int_ok = int_used == int_bytes.size() && int_dst.getIntValue() == 20261125;
        check(int_ok, "GlobalVariable int serialize/load round-trip");

        agv::core::GlobalVariable str_var(agv::core::GlobalVariable::DT_STRING);
        str_var.setString("agile-vision");
        auto str_bytes = str_var.serializeToBytes();
        agv::core::GlobalVariable str_dst(agv::core::GlobalVariable::DT_INT);
        auto str_used = str_dst.loadBytes(str_bytes.data(), str_bytes.size());
        const bool str_ok = str_used == str_bytes.size() && str_dst.getStringValue() == "agile-vision";
        check(str_ok, "GlobalVariable string serialize/load round-trip");

        agv::core::GlobalVariable bool_var(agv::core::GlobalVariable::DT_BOOLEAN);
        bool_var.setBool(true);
        auto bool_bytes = bool_var.serializeToBytes();
        agv::core::GlobalVariable bool_dst(agv::core::GlobalVariable::DT_FLOAT);
        auto bool_used = bool_dst.loadBytes(bool_bytes.data(), bool_bytes.size());
        const bool bool_ok = bool_used == bool_bytes.size() && bool_dst.getBoolValue() == true;
        check(bool_ok, "GlobalVariable bool serialize/load round-trip");

        agv::core::GlobalVariable empty_var(agv::core::GlobalVariable::DT_FLOAT);
        const bool empty_ok = !empty_var.getFloatValue().has_value();
        check(empty_ok, "GlobalVariable getter returns nullopt when value not set");
    }

    void testGlobalVariableMalformed()
    {
        const unsigned int bad_type = 99;
        std::vector<char> bad(sizeof(bad_type));
        std::memcpy(bad.data(), &bad_type, sizeof(bad_type));
        agv::core::GlobalVariable var(agv::core::GlobalVariable::DT_INT);
        check(var.loadBytes(bad.data(), bad.size()) == 0, "GlobalVariable rejects invalid data type");
        check(var.loadBytes(bad.data(), 0) == 0, "GlobalVariable rejects truncated header");
        check(var.loadBytes(nullptr, bad.size()) == 0, "GlobalVariable rejects null buffer");
    }

    void testArrayXNegativeIndex()
    {
        agv::ArrayX<int, 3> arr = {1, 2, 3};
        bool threw = false;
        try
        {
            (void)arr[-1];
        }
        catch(const std::out_of_range&)
        {
            threw = true;
        }
        check(threw, "ArrayX rejects negative index");
    }

}

int main()
{
    testLineRoundTrip();
    testCircleRoundTrip();
    testDataSpecRoundTrip();
    testDataBufferStringBound();
    testMalformedVecInput();
    testGlobalVariableRoundTrip();
    testGlobalVariableMalformed();
    testArrayXNegativeIndex();
    if(g_failures != 0)
    {
        std::printf("SMOKE TEST FAILED: %d case(s)\n", g_failures);
        return 1;
    }
    std::printf("SMOKE TEST PASSED\n");
    return 0;
}
