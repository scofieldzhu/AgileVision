cmake_minimum_required(VERSION 3.21)

project(AgileVision VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# 构建选项（集中声明于顶层）
# ---------------------------------------------------------------------------
option(AGV_BUILD_TESTS "Build the test executable" ON)
option(AGV_REQUIRE_PROTOBUF "Fail configuration when Protobuf is not found" OFF)

# ---------------------------------------------------------------------------
# 安装前缀
# ---------------------------------------------------------------------------
# 命令行 -DCMAKE_INSTALL_PREFIX 或 preset 的 installDir 优先；仅在完全未指定
# 时回退到源码树下的 install 目录，避免覆盖调用方的显式配置。
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install"
        CACHE PATH "Installation prefix" FORCE)
endif()

# 头文件统一安装到 <prefix>/<includedir>/agile_vision/<module>，与源码中
# #include "agile_vision/..." 的路径保持一致。
set(AGV_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/agile_vision")

# ---------------------------------------------------------------------------
# 输出目录（兼容单配置与多配置生成器）
# ---------------------------------------------------------------------------
# $<CONFIG> 在单配置生成器下求值为 CMAKE_BUILD_TYPE，在多配置生成器
# （Visual Studio / Xcode）下求值为当前 configuration；显式写入可保证两种
# 生成器的产物布局一致，均为 bin/<Config>。Debug 的 _d 后缀由各目标的
# DEBUG_POSTFIX 属性按配置自动附加。
set(AGV_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${AGV_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${AGV_OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${AGV_OUTPUT_DIR}")

# ---------------------------------------------------------------------------
# 公共构建配置（INTERFACE 目标，取代目录级全局命令）
# ---------------------------------------------------------------------------
find_package(spdlog CONFIG REQUIRED)

add_library(agv_build_config INTERFACE)
target_compile_features(agv_build_config INTERFACE cxx_std_20)
target_include_directories(agv_build_config INTERFACE
    "${CMAKE_SOURCE_DIR}/source"
)
target_link_libraries(agv_build_config INTERFACE spdlog::spdlog)
target_compile_definitions(agv_build_config INTERFACE
    SPDLOG_COMPILED_LIB
    SPDLOG_SHARED_LIB
)
if(MSVC)
    target_compile_options(agv_build_config INTERFACE /wd4996 /wd4275 /wd4251)
endif()

# ---------------------------------------------------------------------------
# 辅助函数
# ---------------------------------------------------------------------------
# 创建 AgileVision 模块 DLL，统一源文件收集、构建配置、Debug 后缀与安装规则。
# 需并入本模块一同编译的额外源文件（如 basic 中的 system 实现）通过 ARGN 传入。
# 头文件按模块名自动安装到 <prefix>/<includedir>/agile_vision/<module>。
function(agv_add_module module_name)
    file(GLOB agv_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
    file(GLOB agv_headers CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    )

    add_library(${module_name} SHARED ${agv_sources} ${agv_headers} ${ARGN})
    target_link_libraries(${module_name} PUBLIC agv_build_config)
    set_target_properties(${module_name} PROPERTIES DEBUG_POSTFIX "_d")

    string(REPLACE "agile_vision." "" module_short "${module_name}")
    install(TARGETS ${module_name}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
    install(FILES ${agv_headers}
        DESTINATION "${AGV_INSTALL_INCLUDEDIR}/${module_short}"
    )
endfunction()

# ---------------------------------------------------------------------------
# 子模块
# ---------------------------------------------------------------------------
add_subdirectory(source/agile_vision/basic)
add_subdirectory(source/agile_vision/core)
add_subdirectory(source/agile_vision/stdtool)
add_subdirectory(source/agile_vision/package)

if(AGV_BUILD_TESTS)
    add_subdirectory(source/agile_vision/test)
endif()

# 第三方运行时 DLL 由 vcpkg 的 X_VCPKG_APPLOCAL_DEPS_INSTALL 随目标自动安装。
