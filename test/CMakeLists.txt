add_executable(test-timetools
        src/TestBasic.cpp
)

target_include_directories(test-timetools PRIVATE include)
target_link_libraries(test-timetools
        lib-timetools
        GTest::gtest_main)
target_compile_options(test-timetools PRIVATE $<$<CONFIG:Debug>:-g -O0>)

include(GoogleTest)
gtest_discover_tests(test-timetools DISCOVERY_MODE PRE_TEST)