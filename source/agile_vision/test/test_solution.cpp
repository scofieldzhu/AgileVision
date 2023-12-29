#include "test_solution.h"
/*
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *
 *   File: test_solution.cpp
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
#include "test_solution.h"
#include "local_image2d.h"
#include "agile_vision/core/procedure.h"
#include "agile_vision/core/solution.h"
#include "agile_vision/core/engine.h"
#include "ratel/basic/string_proxy.h"
#include "ratel/basic/dbg_tracker.h"
#include "spdlog/spdlog.h"
using namespace agile_vision;
using namespace ratel;
using namespace spdlog;

using UIDGen = StringProxy;

void InitProcess(Process* p)
{
    _AUTO_FUNC_TRACK_
    UIDGen iid_gen;
    ToolPtr local_image = std::make_shared<LocalImage2d>(iid_gen.NewUID().stdStr());    
    local_image->getPropPin(LocalImage2d::PK_P_ImagePath)->mutableDataBuffer().setStringValue("F:\\Program Files\\CKVisionBuilder\\Images\\ImageCalib.bmp");
    local_image->getPropPin(LocalImage2d::PK_P_DirType)->mutableDataBuffer().setIntValue(0);
    p->appendTool(local_image);
}

void InitProcedure(Solution* sln)
{
    _AUTO_FUNC_TRACK_
    UIDGen iid_gen;
    Procedure* pc = sln->createProcedure(iid_gen.NewUID().stdStr());
    InitProcess(pc->mutableRoot());
}

void Test_Solution()
{
    _AUTO_FUNC_TRACK_
    Engine* e = new Engine();
    Solution* sln = new Solution();
    InitProcedure(sln);
    sln->run(e);
}
