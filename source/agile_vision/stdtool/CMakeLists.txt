agv_add_module(agile_vision.stdtool)

target_link_libraries(agile_vision.stdtool PUBLIC
    agile_vision.basic
    agile_vision.core
)

if(WIN32)
    target_compile_definitions(agile_vision.stdtool PRIVATE AGV_STDTOOL_EXPORT)
endif()
