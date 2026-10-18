# The following CMakeLists.txt file is responsible for building and testing jdm-alloc.

cmake_minimum_required(VERSION 3.10)
project(jdm-alloc VERSION 1.0 LANGUAGES C)

# Set the C standard to C11
set(CMAKE_C_STANDARD 11)

set(CMAKE_REQUIRED_LIBRARIES jdm-log)

# Add the source files for the jdm-alloc library
set(SOURCES
    src/jdm-alloc-internal.h
    src/jdm-alloc-arena.c
    src/jdm-alloc-stack.c
    src/jdm-alloc-pool.c
    src/jdm-alloc-freelist.c
    src/jdm-alloc-custom.c
    src/jdm-alloc-unimplemented.c
    src/jdm-alloc.c
)
# Create the jdm-alloc library
add_library(jdm-alloc STATIC ${SOURCES})

# Include the header files for the jdm-alloc library
target_include_directories(jdm-alloc PUBLIC include)

# Link the jdm-log library to the jdm-alloc library
target_link_libraries(jdm-alloc PRIVATE jdm-log)

# Add the test executables
add_executable(test-jdm-alloc tests/test-jdm-alloc.c)
target_link_libraries(test-jdm-alloc PRIVATE jdm-alloc jdm-log)
add_test(NAME test-jdm-alloc COMMAND test-jdm-alloc)

add_executable(test-jdm-alloc-arena tests/test-jdm-alloc-arena.c)
target_link_libraries(test-jdm-alloc-arena PRIVATE jdm-alloc jdm-log)
add_test(NAME test-jdm-alloc-arena COMMAND test-jdm-alloc-arena)

add_executable(test-jdm-alloc-stack tests/test-jdm-alloc-stack.c)
target_link_libraries(test-jdm-alloc-stack PRIVATE jdm-alloc jdm-log)
add_test(NAME test-jdm-alloc-stack COMMAND test-jdm-alloc-stack)

add_executable(test-jdm-alloc-pool tests/test-jdm-alloc-pool.c)
target_link_libraries(test-jdm-alloc-pool PRIVATE jdm-alloc jdm-log)
add_test(NAME test-jdm-alloc-pool COMMAND test-jdm-alloc-pool)

add_executable(test-jdm-alloc-freelist tests/test-jdm-alloc-freelist.c)
target_link_libraries(test-jdm-alloc-freelist PRIVATE jdm-alloc jdm-log)
add_test(NAME test-jdm-alloc-freelist COMMAND test-jdm-alloc-freelist)

# Enable testing
enable_testing()

# Set the CMake build type to Release by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set the output directories for the build artifacts
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set the compiler flags for the Release build type
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Set the compiler flags for the Debug build type
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Set the compiler flags for the Sanitized build type
set(CMAKE_C_FLAGS_SANITIZED "-fsanitize=address -g -O1 -fno-omit-frame-pointer -DDEBUG")

# Set the compiler flags for the RelWithDebInfo build type
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

# Set the compiler flags for the MinSizeRel build type
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")

# Build the jdm-alloc library and the test executable
add_custom_target(build-all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target jdm-alloc
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test-jdm-alloc
    COMMENT "Building jdm-alloc library and test executable"
)

# set the default target to build-all
set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/bin)

# Add a custom target to run the tests
add_custom_target(run-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test-jdm-alloc
    COMMENT "Running tests for jdm-alloc"
)

# build release and run tests
add_custom_target(build-release-and-test
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Release
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Building jdm-alloc library in Release mode and running tests"
)

# set install target for the jdm-alloc library and header files
install(TARGETS jdm-alloc
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)    
install(DIRECTORY include/ DESTINATION include)
install(TARGETS 
    test-jdm-alloc
    test-jdm-alloc-arena
    test-jdm-alloc-stack
    test-jdm-alloc-pool
    test-jdm-alloc-freelist
    RUNTIME DESTINATION bin
)
