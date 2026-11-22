# 基础模块：公共类型 + 内嵌工具实现（system 的动态库加载实现并入本目标编译）。
agv_add_module(agile_vision.basic
    "${CMAKE_CURRENT_SOURCE_DIR}/../system/dynamic_library_loader.cpp"
)

# geometry / system 为纯头文件模块，其声明随基础库导出；定义导出侧宏（其余模块走 dllimport）。
if(WIN32)
    target_compile_definitions(agile_vision.basic PRIVATE
        AGV_BASIC_EXPORT
        AGV_GEOMETRY_EXPORT
        AGV_SYSTEM_EXPORT
    )
endif()

# geometry / system 为纯头文件模块，随基础模块安装，使公共头中的
# "agile_vision/geometry/..."、"agile_vision/system/..." 可被外部消费者解析。
file(GLOB geometry_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../geometry/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../geometry/*.hpp"
)
install(FILES ${geometry_headers} DESTINATION "${AGV_INSTALL_INCLUDEDIR}/geometry")

file(GLOB system_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../system/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../system/*.hpp"
)
install(FILES ${system_headers} DESTINATION "${AGV_INSTALL_INCLUDEDIR}/system")
