set(proj_name "agile_vision.package")

project(${proj_name})

file(GLOB
	 SOURCE_FILES 
	 "*.cpp"
	 "*.h"
	 "*.hpp"
)
add_library(${proj_name} SHARED ${SOURCE_FILES})

find_package(Protobuf REQUIRED)

set_target_properties(${proj_name} PROPERTIES
    DEBUG_POSTFIX _d
)

target_compile_definitions(${proj_name} PRIVATE 
	SPDLOG_COMPILED_LIB
	SPDLOG_SHARED_LIB
	SPDLOG_USE_STD_FORMAT
)

if(WIN32)
	target_compile_definitions(${proj_name} PRIVATE AGILE_VISION_PACKAGE_EXPORT)
endif()

target_link_libraries(${proj_name} agile_vision.basic agile_vision.core)

set(deplibs_dbg spdlogd ratel.basic_d ratel.system_d)
set(deplibs_opt spdlog ratel.basic ratel.system)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
 	target_link_libraries(${proj_name} ${deplibs_dbg})
else()
 	target_link_libraries(${proj_name} ${deplibs_opt})
endif()
message(status "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

install(TARGETS ${proj_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BIN})
install(TARGETS ${proj_name} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIB})
file(GLOB HEADERS "*.h" "*.hpp")
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDE}/package)