add_library(kunpacker "")

target_sources(kunpacker
  PRIVATE
    include/kunpacker.hpp
		src/kunpacker.cpp)
target_include_directories(kunpacker
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(kunpacker PUBLIC cxx_std_17)
set_target_properties(kunpacker PROPERTIES
  VERSION ${PROJECT_VERSION}
  PUBLIC_HEADER include/kunpacker.hpp)
add_library(kunpacker::kunpacker ALIAS kunpacker)

#if(BUILD_TESTING)
#  add_subdirectory(test)
#endif()

# Install
include(GNUInstallDirs)
install(TARGETS kunpacker
  EXPORT ${PROJECT_NAME}Targets
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  #RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
