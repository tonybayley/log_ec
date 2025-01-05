cmake_minimum_required(VERSION 3.22)

project(log_ec)
add_library(log_ec INTERFACE)

# Enable CMake support for C language
enable_language(C)

set(LOG_MAX_CALLBACKS "0" CACHE STRING "Maximum permitted number of logging callback functions. Set to 0 to disable callbacks.")
set(LOG_USE_COLOR "0" CACHE STRING "Set LOG_USE_COLOR to 1 to use ANSI color escape codes, or 0 for monochrome log printing.")

target_compile_definitions(log_ec INTERFACE 
    LOG_MAX_CALLBACKS=${LOG_MAX_CALLBACKS}
	LOG_USE_COLOR=${LOG_USE_COLOR}
)

target_include_directories(log_ec INTERFACE
    src
)

target_sources(log_ec INTERFACE
    src/log_ec.c
)

# -----------------------------------------------------------------------------

# Unit tests
if(PROJECT_IS_TOP_LEVEL)
    # Override printf() and vprintf() with functions that write log messages to a test buffer, for unit tests
    target_compile_definitions(log_ec INTERFACE OVERRIDE_PRINTF)
    target_include_directories(log_ec INTERFACE test)

    # Build TestRunner for unit tests
    include(CTest)
    add_subdirectory(test)
endif()
