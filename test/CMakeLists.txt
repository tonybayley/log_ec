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

foreach(testItem IN LISTS testList)
    add_test(NAME ${testItem} COMMAND TestRunner ${testItem})
endforeach()
