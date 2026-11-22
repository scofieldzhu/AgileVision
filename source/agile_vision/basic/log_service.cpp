
/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: log_service.cpp  
 *   Copyright (c) 2023-2026 scofieldzhu
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
#include "agile_vision/basic/log_service.h"        

namespace agv{    
    namespace{
        // 默认 logger 输出附带源码位置：%s:%# 为短文件名与行号，%! 为函数名。
        std::shared_ptr<spdlog::logger> createDefaultLogger()
        {
            auto logger = spdlog::default_logger();
            logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] [%!] %v");
            return logger;
        }
    }

    namespace details{
        // 默认使用 spdlog 的默认 logger，保证外部通过 SetLogger 注入之前
        // GetLogger() 也能返回有效对象，避免日志宏空指针解引用。
        std::shared_ptr<spdlog::logger> gLogger_ = createDefaultLogger();
        void SetLogger(std::shared_ptr<spdlog::logger> logger)
        {
            gLogger_ = logger;
        }
    }

    spdlog::logger* GetLogger()
    {
        return details::gLogger_.get();
    }
}