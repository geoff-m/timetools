#include "Utility.h"
#include <immintrin.h>
#include <thread>

namespace timetools::detail {
    void __attribute__ ((noinline)) doNothing() {
    }

    uint64_t timeFunctionCallTsc() {
        uint64_t totalTsc = 0;
        constexpr int TRIALS_LOG2 = 10;
        int trial = 1;
        for (; trial <= 1 << TRIALS_LOG2; ++trial) {
            const auto before = __rdtsc();
            doNothing();
            const auto after = __rdtsc();
            totalTsc += after - before;
        }
        const auto average = totalTsc >> TRIALS_LOG2;
        return average;
    }

    uint64_t estimateTscFrequencyHz() {
        const auto overhead = timeFunctionCallTsc();
        constexpr int DURATION_SECONDS = 5;
        const auto duration = std::chrono::seconds(DURATION_SECONDS);
        const auto before = __rdtsc();
        std::this_thread::sleep_for(duration);
        const auto after = __rdtsc();
        const auto tscPerSecond = (after - before - overhead) / DURATION_SECONDS;
        return tscPerSecond;
    }
}
