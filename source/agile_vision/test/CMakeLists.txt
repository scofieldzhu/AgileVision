
set(proj_name "test")

project(${proj_name})

set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/share/opencv4")
find_package(OpenCV REQUIRED)

file(GLOB SOURCE_FILES "${PROJECT_SOURCE_DIR}/*.h" "${PROJECT_SOURCE_DIR}/*.cpp")
add_executable(${proj_name} ${SOURCE_FILES})

set_target_properties(${proj_name} PROPERTIES
    DEBUG_POSTFIX _d
)

target_compile_definitions(${proj_name} PRIVATE 
	SPDLOG_COMPILED_LIB
	SPDLOG_SHARED_LIB
	SPDLOG_USE_STD_FORMAT
)
target_include_directories(${proj_name} PRIVATE ${OpenCV_INCLUDE_DIRS}) # Not needed for CMake >= 2.8.11

target_link_libraries(${proj_name} agile_vision.basic agile_vision.core agile_vision.stdtool ${OpenCV_LIBS})

set(deplibs_dbg spdlogd ratel.basic_d)
set(deplibs_opt spdlog  ratel.basic)
target_link_libraries(${proj_name} debug ${deplibs_dbg} optimized ${deplibs_opt})

install(TARGETS ${proj_name} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)