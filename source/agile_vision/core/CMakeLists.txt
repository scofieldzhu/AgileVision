agv_add_module(agile_vision.core)

target_link_libraries(agile_vision.core PUBLIC agile_vision.basic)

if(WIN32)
    target_compile_definitions(agile_vision.core PRIVATE AGV_CORE_EXPORT)
endif()
