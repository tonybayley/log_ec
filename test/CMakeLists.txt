# Setup compiler settings
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
enable_language(C)

# These flags are used if cmake is called with -DCMAKE_BUILD_TYPE=Profile
set(CMAKE_C_FLAGS_PROFILE "--coverage -g -O0")
set(CMAKE_CXX_FLAGS_PROFILE "--coverage -g -O0")

# Define the default build type for unit tests
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Profile")
endif()
message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

# Test runner application
add_executable(TestRunner test_runner.c)
target_link_libraries(TestRunner PRIVATE log_ec)

target_compile_options(TestRunner PRIVATE
    # compiler warnings
    -Wuninitialized
    -Wundef
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    # sanitizers
    -fsanitize=address
    -fsanitize=leak
    -fsanitize=undefined
    -fno-sanitize-recover
    -fno-omit-frame-pointer
    -static-libasan
)

target_link_options(TestRunner PRIVATE
    # sanitizers
    -fsanitize=address
    -fsanitize=leak
    -fsanitize=undefined
    -fno-sanitize-recover
    -fno-omit-frame-pointer
    -static-libasan
)

# -----------------------------------------------------------------------------

# Add tests

set(testList
    "log_trace message format"
    "log_debug message format"
    "log_info message format"
    "log_warn message format"
    "log_error message format"
    "log_fatal message format with 10 digit timestamp"
    "log message shall be written when lock is free"
    "log message shall not be written when lock is taken"
    "log_off shall disable printing of log messages"
    "log_on shall enable printing of log messages"
    "log message at level set by log_setLevel shall be printed"
    "log message at higher level than set by log_setLevel shall be printed"
    "log message at lower level than set by log_setLevel shall not be printed"
    "when timestamp function is NULL the timestamp value is 0"
    "when callback1 is subscribed with level LOG_INFO then log_info shall invoke callback1"
    "when callback1 is subscribed with level LOG_INFO then log_warn shall invoke callback1"
    "when callback1 is subscribed with level LOG_INFO then log_debug shall not invoke callback1"
    "when callback1 and callback2 are subscribed both callbacks shall be invoked"
    "given 2 subscribed callbacks when callback1 is unsubscribed only callback2 shall be invoked"
    "given callback1 is subscribed the subscription shall be overwritten when resubscribed"
    "given 2 subscribed callbacks an attempt to subscribe a third callback shall fail"
    "given 2 subscribed callbacks when callback1 is unsubscribed callback3 subscription succeeds"
    "given 2 subscribed callbacks when callback2 is unsubscribed callback3 subscription succeeds"
)

LIST(LENGTH testList testListLen)

foreach(testNumber RANGE 1 ${testListLen})
    MATH(EXPR listIndex "${testNumber} - 1")
    LIST(GET testList ${listIndex} testName)
    if(CMAKE_BUILD_TYPE STREQUAL "Profile")
        # Generate test coverage report for Profile build
        add_test(NAME ${testName}
                COMMAND bash -c " \
                    ${CMAKE_CURRENT_BINARY_DIR}/TestRunner \"${testName}\" && \
                    gcovr --root ${PROJECT_SOURCE_DIR} \
                        --json test-${testNumber}.json ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/TestRunner.dir/__/src"
        )
        set_tests_properties("${testName}" PROPERTIES
                            FIXTURES_REQUIRED TestFixture
        )
    else()
        # Run tests without code coverage for Release build
        add_test(NAME ${testName} COMMAND TestRunner ${testName})
    endif()
endforeach()

if(CMAKE_BUILD_TYPE STREQUAL "Profile")
    # Generate test coverage report for Profile build
    add_test(NAME "Test coverage report (build/test/coverage.html)"
            COMMAND bash -c "gcovr --root ${PROJECT_SOURCE_DIR} \
                                    --json-add-tracefile \"${CMAKE_CURRENT_BINARY_DIR}/test-*.json\" \
                                    --html-details ${CMAKE_CURRENT_BINARY_DIR}/coverage.html"
    )
    # setting the FIXTURES_CLEANUP property ensures that this runs after all other tests are complete
    set_tests_properties("Test coverage report (build/test/coverage.html)" PROPERTIES
                        FIXTURES_CLEANUP TestFixture
    )
endif()
