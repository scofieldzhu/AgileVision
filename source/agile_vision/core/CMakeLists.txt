set(proj_name "agile_vision.core")

project(${proj_name})

file(GLOB
	 SOURCE_FILES 
	 "*.cpp"
	 "*.h"
	 "*.hpp"
)
add_library(${proj_name} SHARED ${SOURCE_FILES})

set_target_properties(${proj_name} PROPERTIES
    DEBUG_POSTFIX _d
)

target_compile_definitions(${proj_name} PRIVATE 
	SPDLOG_COMPILED_LIB
	SPDLOG_SHARED_LIB
	SPDLOG_USE_STD_FORMAT
)

if(WIN32)
	target_compile_definitions(${proj_name} PRIVATE AGILE_VISION_CORE_EXPORT)
endif()

target_link_libraries(${proj_name} agile_vision.basic)

set(deplibs_dbg spdlogd ratel.basic_d)
set(deplibs_opt spdlog ratel.basic)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
 	target_link_libraries(${proj_name} ${deplibs_dbg})
else()
 	target_link_libraries(${proj_name} ${deplibs_opt})
endif()
message(status "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
#target_link_libraries(${proj_name} debug ${deplibs_dbg} optimized ${deplibs_opt})

# if(WIN32)
# 	set_target_properties(${PROJECT_NAME} PROPERTIES
# 		PREFIX "agile_vision."
# 		SUFFIX ".dll"
# 	)
# endif()

install(TARGETS ${proj_name} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(TARGETS ${proj_name} ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
file(GLOB HEADERS "*.h" "*.hpp")
install(FILES ${HEADERS} DESTINATION ${CMAKE_ISNTALL_INCLUDE}/core)



