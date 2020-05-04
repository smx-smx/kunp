add_executable(kunp)
target_sources(kunp
  PRIVATE
   kallisto.h kunp.cpp
   mfile.h mfile.c)
target_compile_features(kunp PUBLIC cxx_std_17)
set_target_properties(kunp PROPERTIES
  VERSION ${PROJECT_VERSION}
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO)
add_executable(kunpacker::kunp ALIAS kunp)

if(BUILD_TESTING)
  add_test(NAME kunpTest COMMAND kunp)
endif()

# Install
include(GNUInstallDirs)
install(TARGETS kunp
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
