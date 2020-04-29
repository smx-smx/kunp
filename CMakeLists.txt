cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)

add_executable(kunp
	mfile.c
	kunp.cpp
)

if(WIN32)
	set(BUILD_SHARED_LIBS OFF)
	add_subdirectory(mman-win32)
	target_link_libraries(kunp mman)
endif(WIN32)
