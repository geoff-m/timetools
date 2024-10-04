#include <stdexcept>

#include "timetools.h"
#include <cpuid.h>
#include <immintrin.h>
#include <thread>
#include <chrono>
#include <iostream>

//extern bool hasGoodCpuTimer();

extern "C" {
extern void busyWait(uint64_t nanosecondsToWait);
}

namespace timetools {
    static bool hasTpause() {
        int32_t eax, ebx, ecx, edx;
        __cpuid(7, eax, ebx, ecx, edx);
        bool yes = ecx & (1 << 5);
        return yes;
    }

    static bool hasGoodCpuTimer() {
        uint32_t eax, ebx, ecx, edx;
        if (!__get_cpuid(0x15, &eax, &ebx, &ecx, &edx)) {
            return false;
        }
        return eax != 0 && ebx != 0 && ecx != 0;
    }

    void Waiter::rdtscLoopWait(const uint64_t nanosecondsToWait) const {
        const auto tsc_initial = __rdtsc();
        const uint64_t tsc_to_wait = ((nanosecondsToWait * tscPerNanosecond_shl25) >> 25) - overheadTsc;
        const auto tsc_final = tsc_initial + tsc_to_wait;

        while (tsc_final > __rdtsc()) {
            __pause();
        }
    }

    void Waiter::tpauseLoopWait(const uint64_t nanosecondsToWait) {
        const auto tsc_initial = __rdtsc();
        const uint64_t tsc_to_wait = ((nanosecondsToWait * tscPerNanosecond_shl25) >> 25) - overheadTsc;
        const auto tsc_final = tsc_initial + tsc_to_wait;

        while (tsc_final > __rdtsc()) {
            _tpause(0, tsc_final);
            _mm_lfence();
        }
    }

    void Waiter::nanosleepWait(uint64_t nanosecondsToWait) {
        timespec stopTime;
        clock_gettime(CLOCK_MONOTONIC, &stopTime);
        constexpr long NANOSECONDS_PER_SECOND = 1000000000;
        stopTime.tv_sec += nanosecondsToWait / NANOSECONDS_PER_SECOND;
        stopTime.tv_nsec += nanosecondsToWait % NANOSECONDS_PER_SECOND;
        if (stopTime.tv_nsec >= NANOSECONDS_PER_SECOND) {
            stopTime.tv_nsec -= NANOSECONDS_PER_SECOND;
            stopTime.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &stopTime, nullptr);
    }

    void Waiter::systemClockBusyWait(uint64_t nanosecondsToWait) {
        const auto stopTime = std::chrono::system_clock::now() + std::chrono::nanoseconds(nanosecondsToWait);
        while (std::chrono::system_clock::now() < stopTime);
    }

    void Waiter::busyWait(const uint64_t nanosecondsToWait) {
        tpauseLoopWait(nanosecondsToWait);
    }
}
