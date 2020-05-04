add_executable(kunpackerbin)
target_sources(kunpackerbin
  PRIVATE
    src/main.cpp)
target_compile_features(kunpackerbin PUBLIC cxx_std_17)
set_target_properties(kunpackerbin PROPERTIES
  VERSION ${PROJECT_VERSION}
	CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF)
# note: macOS is APPLE and also UNIX !
if(APPLE)
  set_target_properties(kunpackerbin PROPERTIES
    INSTALL_RPATH "@loader_path/../lib")
elseif(UNIX AND NOT APPLE)
  set_target_properties(kunpackerbin PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib")
endif()
target_link_libraries(kunpackerbin PRIVATE kunpacker::kunpacker)
add_executable(kunpacker::kunpackerbin ALIAS kunpackerbin)

if(BUILD_TESTING)
  add_test(NAME kunpackerbinTest COMMAND kunpackerbin)
endif()

# Install
include(GNUInstallDirs)
install(TARGETS kunpackerbin
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
