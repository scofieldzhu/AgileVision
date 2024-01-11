/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: test_solution.cpp  
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

#include "test_solution.h"
#include "agile_vision/core/procedure.h"
#include "agile_vision/core/solution.h"
#include "agile_vision/core/tool.h"
#include "ratel/basic/string_proxy.h"
#include "ratel/basic/dbg_tracker.h"
#include "agile_vision/core/tool_factory.h"
#include "spdlog/spdlog.h"
using namespace agile_vision;
using namespace ratel;
using namespace std::filesystem;

using UIDGen = StringProxy;

namespace {
    ToolFactory sToolFactory;
    std::string sLocalImage2dClsId = "dbd43f17-8e6d-4776-b383-a73b75b20601";
    constexpr PinKey PK_P_ImagePath = "ImagePath";
    constexpr PinKey PK_P_DirType = "DirType";
    constexpr PinKey PK_O_ImageData = "ImageData";
    path sExecDir;
}

void InitToolFactory()
{
    _AUTO_FUNC_TRACK_
    path tool_root_dir = sExecDir;
    for(const auto& it : directory_iterator(tool_root_dir)){
        auto cur_path = it.path();
        if(is_regular_file(cur_path) && cur_path.extension() == ".agvt"){
            auto load_status = sToolFactory.loadToolFile(cur_path);
            spdlog::trace("Load tool:[{}] {}!", cur_path.filename().string(), load_status ? "Successfully" : "Failed");
        }
    }
}

void InitProcess(Process* p)
{
    _AUTO_FUNC_TRACK_
    path img_path = sExecDir;
    img_path /= "ImageCalib.bmp";
    UIDGen iid_gen;
    ToolPtr local_image = sToolFactory.createTool(sLocalImage2dClsId, iid_gen.NewUID().stdStr());
    local_image->getPropPin(PK_P_ImagePath)->mutableDataBuffer().setStringValue(img_path.string().c_str());
    local_image->getPropPin(PK_P_DirType)->mutableDataBuffer().setIntValue(0);
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
    char exe_path[MAX_PATH] = {0};
    GetModuleFileName(NULL, exe_path, MAX_PATH);
    sExecDir = path(exe_path).parent_path();
    InitToolFactory();
    Solution* sln = new Solution(3);
    InitProcedure(sln);
    sln->run();
}
