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

    void Waiter::rdtscPauseLoopWait(const uint64_t nanosecondsToWait) const {
        const auto tsc_initial = __rdtsc();
        const uint64_t tsc_to_wait = ((nanosecondsToWait * tscPerNanosecond_shl25) >> 25) - overheadTsc;
        const auto tsc_final = tsc_initial + tsc_to_wait;

        while (tsc_final > __rdtsc()) {
            __pause();
        }
    }

    void Waiter::rdtscPauseUnrolledWait(const uint64_t nanosecondsToWait) const {
        const auto tsc_initial = __rdtsc();
        const uint64_t tsc_to_wait = ((nanosecondsToWait * tscPerNanosecond_shl25) >> 25) - overheadTsc;
        const auto tsc_final = tsc_initial + tsc_to_wait;

        while (true) {
            if (tsc_final <= __rdtsc())
                break;
            __pause();
            if (tsc_final <= __rdtsc())
                break;
            __pause();
            if (tsc_final <= __rdtsc())
                break;
            __pause();
            if (tsc_final <= __rdtsc())
                break;
            __pause();
        }
    }

    void Waiter::rdtscpWait(uint64_t nanosecondsToWait) const {
        unsigned int cpu_old;
        auto tsc_initial = __rdtscp(&cpu_old);
        uint64_t tsc_to_wait = ((nanosecondsToWait * tscPerNanosecond_shl25) >> 25);

        while (true) {
            unsigned int cpu_new;
            const auto now = __rdtscp(&cpu_new);
            if (now - tsc_initial >= tsc_to_wait)
                return;
            // if (cpu_new != cpu_old) [[unlikely]] {
            //     // Relativize the wait to the new CPU's TSC.
            //     // See https://codebrowser.dev/linux/linux/arch/x86/lib/delay.c.html#delay_tsc
            //     // Subtract the amount we've already waited.
            //     tsc_to_wait -= now - tsc_initial;
            //     cpu_old = cpu_new;
            //     // Start using the new CPU's TSC.
            //     tsc_initial = now;
            // }
            __pause();
        }
    }

    void Waiter::tpauseLoopWait(const uint64_t nanosecondsToWait) {
        const auto tsc_initial = __rdtsc();
        const uint64_t tsc_to_wait = ((nanosecondsToWait * tscPerNanosecond_shl25) >> 25) - overheadTsc;
        const auto tsc_final = tsc_initial + tsc_to_wait;

        while (tsc_final > __rdtsc()) {
            _tpause(1, tsc_final);
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

    void Waiter::threadSleep(uint64_t nanosecondsToWait) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(nanosecondsToWait));
    }

    void Waiter::busyWait(const uint64_t nanosecondsToWait) {
        rdtscPauseLoopWait(nanosecondsToWait);
        //rdtscpWait(nanosecondsToWait);
    }
}
