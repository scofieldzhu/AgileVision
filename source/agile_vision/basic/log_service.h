/*
 *   AgileVision - generic vision framework.
 *
 *   File: log_service.h
 *   Copyright (c) 2023 ~ 2026 scofieldzhu. All rights reserved.
 *
 *   MIT License
 */

#ifndef __log_service_h__
#define __log_service_h__

// 项目统一日志封装（CODING_STANDARD.md §12）。
// 六个 AGV_LOG_* 宏经 agv::GetLogger() 记录，自动附带文件、行号与函数名；
// 日志消息一律使用英文。业务代码禁止直接调用 spdlog::xxx。
#include "spdlog/spdlog.h"
#include "agile_vision/basic/module_export.h"

namespace agv{    
    namespace details{
        AGV_BASIC_API void SetLogger(std::shared_ptr<spdlog::logger> logger);
    }
    AGV_BASIC_API spdlog::logger* GetLogger();
}

#define AGV_LOG_TRACE(...) agv::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::trace, __VA_ARGS__)
#define AGV_LOG_DEBUG(...) agv::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::debug, __VA_ARGS__)
#define AGV_LOG_INFO(...) agv::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::info, __VA_ARGS__)
#define AGV_LOG_WARN(...) agv::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::warn, __VA_ARGS__)
#define AGV_LOG_ERROR(...) agv::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::err, __VA_ARGS__)
#define AGV_LOG_CRITICAL(...) agv::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::critical, __VA_ARGS__)

#endif
