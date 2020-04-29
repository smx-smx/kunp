cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

add_executable(kunp kunp.c)

if(WIN32)
	set(BUILD_SHARED_LIBS OFF)
	add_subdirectory(mman-win32)
	target_link_libraries(kunp mman)
endif(WIN32)
