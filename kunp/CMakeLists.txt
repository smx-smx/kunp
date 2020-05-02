add_executable(kunp)
target_sources(kunp
  PRIVATE
   kallisto.h kunp.cpp
   mfile.h mfile.c)
set_target_properties(kunp PROPERTIES
  VERSION ${PROJECT_VERSION})
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
