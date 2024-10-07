#include <algorithm>
#include <cstring>

#include "timetools.h"
#include "Utility.h"
#include <immintrin.h>
#include <thread>
#include <cassert>
#include <vector>

namespace timetools {

    TimerFactory::TimerFactory() {
        const auto coreCount = std::thread::hardware_concurrency();
        tscPerNanosecond_shl25perCore = new uint64_t[coreCount];
        std::memset(tscPerNanosecond_shl25perCore, 0, coreCount * sizeof(uint64_t));
    }

    uint64_t TimerFactory::getTscRateForCurrentCore() {
        unsigned int coreId;
        __rdtscp(&coreId);
        const auto existingTiming = tscPerNanosecond_shl25perCore[coreId];
        if (existingTiming != 0)
            return existingTiming;
        const auto tscPerSecond = detail::estimateTscFrequencyHz();

        // (x << 16) / 1953125ll is the same as (x << 25) / 1000000000ll.
        const auto tscPerNanosecond_shl25 = static_cast<int64_t>((tscPerSecond << 16) / 1953125ll);

        return tscPerNanosecond_shl25perCore[coreId] = tscPerNanosecond_shl25;
    }

    Stopwatch TimerFactory::createStopwatch() {
        // Assume the stopwatch will be used on the current core.
        return Stopwatch(getTscRateForCurrentCore());
    }

    Waiter TimerFactory::createWaiter() {
        // Assume the waiter will be used on the current core.
        return Waiter(getTscRateForCurrentCore());
    }

    void Stopwatch::start() {
        startTsc = __rdtscp(&startCpu);
        _mm_lfence();
    }

    void Stopwatch::stop() {
        unsigned int stopCpu;
        _mm_mfence();
        const auto stopTsc = __rdtscp(&stopCpu);
        assert(startCpu == stopCpu); // This stopwatch is unreliable if the CPU changes.
        elapsedTsc += stopTsc - startTsc;
    }

    void Stopwatch::reset() {
        elapsedTsc = 0;
    }

    uint64_t Stopwatch::getElapsedNanoseconds() const {
        return (elapsedTsc * nanosecondPerTsc_shr16) >> 16;
    }
}
