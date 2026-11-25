# 测试程序（当前未使用 OpenCV，保留为后续视觉测试的扩展入口）。
file(GLOB test_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# 目标名不使用保留字 test（CTest 启用后该名被保留），改名为 agv_demo。
add_executable(agv_demo ${test_sources})
target_link_libraries(agv_demo PRIVATE
    agv_build_config
    agile_vision.basic
    agile_vision.core
    agile_vision.stdtool
)

# 多配置下由 DEBUG_POSTFIX 自动区分 agv_demo_d.exe / agv_demo.exe。
set_target_properties(agv_demo PROPERTIES DEBUG_POSTFIX "_d")

install(TARGETS agv_demo RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
