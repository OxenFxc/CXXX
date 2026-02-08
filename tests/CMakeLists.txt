# Add tests
enable_testing()

set(TEST_SOURCES
    test_chunk.cpp
    test_compiler.cpp
    test_scanner.cpp
    test_vm.cpp
    test_vm_ops.cpp
    test_comparison.cpp
    test_control_flow.cpp
    test_sugar.cpp
    test_functions.cpp
)

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE libcxxx)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
