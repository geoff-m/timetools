#pragma once
#include <cstdint>
#include <iostream>

namespace timetools {
    int setThisThreadAffinity(int cpu);
    int setThisThreadFifoRealtimePriority(int priority);

    class Stopwatch;
    class Waiter;

    class TimerFactory {
        // core number x tscPerNanosecond_shl25
        uint64_t *tscPerNanosecond_shl25perCore;

        uint64_t getTscRateForCurrentCore();

        uint64_t startStopOverheadTsc = -1;

        uint64_t estimateStartStopOverheadTsc();

    public:
        TimerFactory();

        Stopwatch createStopwatch();

        Waiter createWaiter();
    };

    class Stopwatch {
        friend class TimerFactory;
        uint64_t nanosecondPerTsc_shr16;
        uint64_t startTsc = 0;
        uint64_t elapsedTsc = 0;
        uint64_t overheadTsc = 0;
        unsigned int startCpu;

        explicit Stopwatch(uint64_t tscPerNanosecond_shl25,
                           uint64_t overheadTsc)
            : overheadTsc(overheadTsc) {
            //std::cout << "Stopwatch's TSCs per nanosecond: " << (tscPerNanosecond_shl25 >> 25) << "\n";
            const auto asDouble = static_cast<double>(tscPerNanosecond_shl25);
            const auto unshifted = asDouble / (1 << 25);
            const auto nanosPerTsc = 1 / unshifted;
            const auto shiftedLeft = nanosPerTsc * (1 << 16);
            nanosecondPerTsc_shr16 = static_cast<uint64_t>(shiftedLeft);
        }

    public:
        void start();

        void stop();

        void reset();

        [[nodiscard]] uint64_t getElapsedNanoseconds() const;
    };

    class Waiter {
        friend class TimerFactory;
        uint64_t tscPerNanosecond_shl25; // TSC per nanosecond * 2^25
        uint64_t overheadTsc = 0;

        void rdtscLoopWait(uint64_t nanosecondsToWait) const;

        void tpauseLoopWait(uint64_t nanosecondsToWait);

        void systemClockBusyWait(uint64_t nanosecondsToWait);
        void nanosleepWait(uint64_t nanosecondsToWait);

        explicit Waiter(uint64_t tscPerNanosecond_shl25,
                        uint64_t overheadTsc)
            : tscPerNanosecond_shl25(tscPerNanosecond_shl25),
              overheadTsc(overheadTsc) {
            //std::cout << "Waiter's TSCs per nanosecond: " << (tscPerNanosecond_shl25 >> 25) << "\n";
            //std::cout << "Waiter's overhead (TSC): " << overheadTsc << "\n";
        }
    public:
        void busyWait(uint64_t nanosecondsToWait);
    };
}
