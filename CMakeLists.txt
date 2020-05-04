cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(KUnpacker VERSION 0.1 LANGUAGES CXX C)
message(STATUS "project: ${PROJECT_NAME}")
message(STATUS "version: ${PROJECT_VERSION}")

include(CTest)

# Legacy project
#add_subdirectory(kunp)
#if(WIN32)
#  set(BUILD_SHARED_LIBS OFF)
#  add_subdirectory(mman-win32)
#  target_link_libraries(kunp mman)
#endif(WIN32)

# Mizux fork
add_subdirectory(libkunpacker)
add_subdirectory(kunpacker)

# Install
include(GNUInstallDirs)
install(EXPORT KUnpackerTargets
  NAMESPACE kunpacker::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KUnpacker
  COMPONENT Devel)
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/KUnpackerConfig.cmake.in
  "${PROJECT_BINARY_DIR}/KUnpackerConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/KUnpacker"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)
write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/KUnpackerConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion)
install(
  FILES
  "${PROJECT_BINARY_DIR}/KUnpackerConfig.cmake"
  "${PROJECT_BINARY_DIR}/KUnpackerConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/KUnpacker"
  COMPONENT Devel)
