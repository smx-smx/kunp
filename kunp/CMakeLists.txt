add_executable(kunp)
target_sources(kunp
	PRIVATE
	  kunp.c
		kallisto.h)
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
