add_llbuild_unittest(BasicTests
  BinaryCodingTests.cpp
  Defer.cpp
  FileSystemTest.cpp
  POSIXEnvironmentTest.cpp
  SerialQueueTest.cpp
  ShellUtilityTest.cpp
  ../BuildSystem/TempDir.cpp
  )

target_link_libraries(BasicTests PRIVATE
  llbuildBasic
  llvmSupport)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(BasicTests PRIVATE
    curses)
endif()

# Add widenPath performance benchmark (Windows only)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  add_executable(widenPathBenchmark
    widenPathBenchmark.cpp
    )
  
  target_link_libraries(widenPathBenchmark PRIVATE
    llbuildBasic
    llvmSupport)
  
  # Set output directory to make it easy to find
  set_target_properties(widenPathBenchmark PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
