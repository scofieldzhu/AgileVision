# 测试程序（当前未使用 OpenCV，保留为后续视觉测试的扩展入口）。
file(GLOB test_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(test ${test_sources})
target_link_libraries(test PRIVATE
    agv_build_config
    agile_vision.basic
    agile_vision.core
    agile_vision.stdtool
)

# 多配置下由 DEBUG_POSTFIX 自动区分 test_d.exe / test.exe。
set_target_properties(test PROPERTIES DEBUG_POSTFIX "_d")

install(TARGETS test RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
