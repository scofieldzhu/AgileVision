# protobuf 相关代码尚未接入构建，默认不强制要求本机安装 Protobuf。
# 后续启用 .proto 生成/序列化时，可在配置时传入 -DAGV_REQUIRE_PROTOBUF=ON。
if(AGV_REQUIRE_PROTOBUF)
    find_package(Protobuf REQUIRED)
else()
    find_package(Protobuf QUIET)
endif()

agv_add_module(agile_vision.package)

target_link_libraries(agile_vision.package PUBLIC
    agile_vision.basic
    agile_vision.core
)

if(WIN32)
    target_compile_definitions(agile_vision.package PRIVATE AGV_PACKAGE_EXPORT)
endif()
