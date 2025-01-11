# Setup compiler settings
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
enable_language(C)

# Define the default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

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
    # code coverage
    --coverage
)

target_link_options(TestRunner PRIVATE
    # sanitizers
    -fsanitize=address
    -fsanitize=leak
    -fsanitize=undefined
    -fno-sanitize-recover
    -fno-omit-frame-pointer
    -static-libasan
    # code coverage
    --coverage
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
)

LIST(LENGTH testList testListLen)

foreach(testNumber RANGE 1 ${testListLen})
    MATH(EXPR listIndex "${testNumber} - 1")
    LIST(GET testList ${listIndex} testName)
    add_test(NAME ${testName}
             COMMAND bash -c " \
                ${CMAKE_CURRENT_BINARY_DIR}/TestRunner \"${testName}\" && \
                gcovr --root ${PROJECT_SOURCE_DIR} \
                      --json test-${testNumber}.json ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/TestRunner.dir/__/src"
    )
    set_tests_properties("${testName}" PROPERTIES
                         FIXTURES_REQUIRED TestFixture
    )
endforeach()

# generate test coverage report
add_test(NAME "Test coverage report (build/test/coverage.html)"
         COMMAND bash -c "gcovr --root ${PROJECT_SOURCE_DIR} \
                                --json-add-tracefile \"${CMAKE_CURRENT_BINARY_DIR}/test-*.json\" \
                                --html-details ${CMAKE_CURRENT_BINARY_DIR}/coverage.html"
)
# setting the FIXTURES_CLEANUP property ensures that this runs after all other tests are complete
set_tests_properties("Test coverage report (build/test/coverage.html)" PROPERTIES
                     FIXTURES_CLEANUP TestFixture
)
