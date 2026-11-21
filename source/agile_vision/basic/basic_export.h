/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: basic_export.h  
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

#ifndef __basic_export_h__
#define __basic_export_h__

#include "agile_vision/basic/export_macro.h"

// AgileVision 基础模块导出宏
#ifdef PLATFORM_WIN
    #ifdef AGV_BASIC_EXPORT
        #define AGV_BASIC_API RATEL_WIN_DLL_EXPORT
    #else
        #define AGV_BASIC_API RATEL_WIN_DLL_IMPORT
    #endif
#endif

// 内嵌 Ratel 基础工具导出宏（RATEL_BASIC_EXPORTS 由 basic 目标定义）
#ifdef PLATFORM_WIN
    #ifdef RATEL_BASIC_EXPORTS
        #define RATEL_BASIC_API RATEL_WIN_DLL_EXPORT
    #else
        #define RATEL_BASIC_API RATEL_WIN_DLL_IMPORT
    #endif
#endif

#endif
