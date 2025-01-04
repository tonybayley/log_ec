# Unit tests
add_executable(TestRunner test_runner.c)
target_link_libraries(TestRunner PRIVATE log_ec)

add_test(
    NAME "test_log_trace_messageFormat"
    COMMAND TestRunner 0
)

add_test(
    NAME "test_log_debug_messageFormat"
    COMMAND TestRunner 1
)
