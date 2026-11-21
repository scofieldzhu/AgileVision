/*
 *   AgileVision - generic vision framework.
 *
 *   File: log.h
 *   Copyright (c) 2023 ~ 2026 scofieldzhu. All rights reserved.
 *
 *   MIT License
 */

#ifndef __log_h__
#define __log_h__

// 项目统一日志封装（CODING_STANDARD.md §12）。
// 基于 spdlog 的 SPDLOG_* 宏，自动附带文件、行号与函数名等上下文；
// 日志消息一律使用英文。业务代码禁止直接调用 spdlog::xxx。
#include "spdlog/spdlog.h"

#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

#endif
