cmake_minimum_required(VERSION 3.21)

project(AgileVision VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
enable_testing()

# ---------------------------------------------------------------------------
# 构建选项（集中声明于顶层）
# ---------------------------------------------------------------------------
option(AGV_BUILD_TESTS "Build the test executable" ON)
option(AGV_REQUIRE_PROTOBUF "Fail configuration when Protobuf is not found" OFF)
# 序列化后端：proxy 使用内嵌几何代理（默认、无外部依赖）；protobuf 为预留实现。
set(AGV_SERIALIZER_BACKEND "proxy" CACHE STRING "Serialization backend: proxy | protobuf")
set_property(CACHE AGV_SERIALIZER_BACKEND PROPERTY STRINGS proxy protobuf)

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
# 构建树使用源码路径，安装树使用安装头文件目录，保证导出目标可被外部消费者解析。
target_include_directories(agv_build_config INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/source>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(agv_build_config INTERFACE spdlog::spdlog)
target_compile_definitions(agv_build_config INTERFACE
    SPDLOG_COMPILED_LIB
    SPDLOG_SHARED_LIB
)
if(AGV_SERIALIZER_BACKEND STREQUAL "protobuf")
    message(WARNING "protobuf serialization backend is reserved but not implemented yet; falling back to proxy.")
    target_compile_definitions(agv_build_config INTERFACE AGV_SERIALIZER_PB)
else()
    target_compile_definitions(agv_build_config INTERFACE AGV_SERIALIZER_PROXY)
endif()
if(MSVC)
    target_compile_options(agv_build_config INTERFACE /wd4996 /wd4275 /wd4251)
endif()

# ---------------------------------------------------------------------------
# 辅助函数
# ---------------------------------------------------------------------------
# 创建 AgileVision 模块 DLL，统一源文件收集、构建配置、Debug 后缀与安装规则。
# 需并入本模块一同编译的额外源文件（如 basic 中的 system 实现）通过 ARGN 传入。
# 头文件按模块名自动安装到 <prefix>/<includedir>/agile_vision/<module>。
# NO_EXPORT：占位模块（无导出符号、无 import lib）不纳入包导出，避免 find_package 引用缺失的 .lib。
function(agv_add_module module_name)
    cmake_parse_arguments(AGV_MODULE "NO_EXPORT" "" "" ${ARGN})

    file(GLOB agv_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
    file(GLOB agv_headers CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    )

    add_library(${module_name} SHARED ${agv_sources} ${agv_headers} ${AGV_MODULE_UNPARSED_ARGUMENTS})
    target_link_libraries(${module_name} PUBLIC agv_build_config)
    set_target_properties(${module_name} PROPERTIES DEBUG_POSTFIX "_d")

    string(REPLACE "agile_vision." "" module_short "${module_name}")
    if(AGV_MODULE_NO_EXPORT)
        install(TARGETS ${module_name}
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        )
    else()
        install(TARGETS ${module_name} EXPORT AgileVisionTargets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        )
    endif()
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
    add_subdirectory(source/agile_vision/smoke_test)
endif()

# ---------------------------------------------------------------------------
# 包配置导出：消费者可 find_package(AgileVision CONFIG) 后链接 AgileVision::* 目标。
# ---------------------------------------------------------------------------
install(TARGETS agv_build_config EXPORT AgileVisionTargets)

install(EXPORT AgileVisionTargets
    FILE AgileVisionTargets.cmake
    NAMESPACE AgileVision::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/AgileVision"
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/AgileVisionConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/AgileVisionConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/AgileVision"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/AgileVisionConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/AgileVisionConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/AgileVisionConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/AgileVision"
)

# 第三方运行时 DLL 由 vcpkg 的 X_VCPKG_APPLOCAL_DEPS_INSTALL 随目标自动安装。
