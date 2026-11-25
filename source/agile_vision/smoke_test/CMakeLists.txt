# 自包含冒烟测试：非交互，覆盖序列化往返与反序列化边界。
add_executable(agv_smoke_test smoke_test.cpp)
target_link_libraries(agv_smoke_test PRIVATE
    agv_build_config
    agile_vision.basic
    agile_vision.core
)
set_target_properties(agv_smoke_test PROPERTIES DEBUG_POSTFIX "_d")

enable_testing()
add_test(NAME agv_smoke_test COMMAND agv_smoke_test)
