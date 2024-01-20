cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(proj_name "AgileVision")

message(status "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

project (${proj_name})

set(trd_root "${CMAKE_SOURCE_DIR}/third_party")
set(trd_incl "${trd_root}/include")
set(trd_bin "${trd_root}/bin")
set(trd_lib "${trd_root}/lib")
set(build_bin "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${build_bin})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${build_bin})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${build_bin})

include_directories(
	${CMAKE_SOURCE_DIR}/source
	${trd_incl}
)

link_directories(
	${trd_lib} 
)

add_compile_options(/wd4996 /wd4275 /wd4251)

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")

set(CMAKE_INSTALL_INCLUDE "${CMAKE_INSTALL_PREFIX}/include/agile_vision")
set(CMAKE_INSTALL_LIB "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_BIN "${CMAKE_INSTALL_PREFIX}/bin")

add_subdirectory(source/agile_vision/basic)

add_subdirectory(source/agile_vision/core)

add_subdirectory(source/agile_vision/stdtool)

add_subdirectory(source/agile_vision/package)

add_subdirectory(source/agile_vision/test)

# install third-party libraries
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	install(FILES ${trd_bin}/spdlogd.dll DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
else()
	install(FILES ${trd_bin}/spdlog.dll DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif()
