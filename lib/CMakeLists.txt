add_library(lib-timetools
        include/timetools.h
        src/Stopwatch.cpp
        src/busyWait.asm
        src/Waiter.cpp
        src/EstimateTscFrequency.cpp
        include/Utility.h
        src/setThisThreadAffinity.cpp
)

target_include_directories(lib-timetools PRIVATE include)

target_compile_options(lib-timetools PRIVATE $<$<CONFIG:Debug>:-g -O1 -march=native>)
target_compile_options(lib-timetools PRIVATE $<$<CONFIG:Release>:-O3 -march=native>)

SET(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")