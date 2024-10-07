#include <gtest/gtest.h>

#include "../../lib/include/timetools.h"

#include <chrono>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <immintrin.h>

class Stopwatch {
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    int64_t elapsedNanoseconds = 0;

public:
    void start() {
        elapsedNanoseconds = 0;
        startTime = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        const auto stopTime = std::chrono::high_resolution_clock::now();
        elapsedNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime).count();
    }

    [[nodiscard]] int64_t getElapsedNanoseconds() const {
        return elapsedNanoseconds;
    }
};

void assertSuccess(const std::function<int()> &function) {
    const auto value = function();
    const auto error = errno;
    const auto errorString = strerror(error);
    ASSERT_EQ(0, value);
}

double getAverageErrorPercent(timetools::TimerFactory &factory, long waitNanoseconds, long trialCount) {
    assertSuccess([]() { return timetools::setThisThreadAffinity(3); });
    assertSuccess([]() { return timetools::setThisThreadFifoRealtimePriority(95); });
    auto waiter = factory.createWaiter();
    long totalAbsErrorNs = 0;
    auto stopwatch = factory.createStopwatch();
    for (long trial = 1; trial <= trialCount; ++trial) {
        stopwatch.reset();
        stopwatch.start();
        waiter.busyWait(waitNanoseconds);
        stopwatch.stop();
        const auto actualWaitTimeNs = static_cast<int64_t>(stopwatch.getElapsedNanoseconds());
        const auto trialErrorNs = std::abs(waitNanoseconds - actualWaitTimeNs);
        totalAbsErrorNs += trialErrorNs;
        const auto timeErrorPercent = 100.0 * trialErrorNs / waitNanoseconds;
        //  std::stringstream ss;
        //  ss << std::setprecision(2) << std::fixed;
        //  ss << "Tried to wait " << waitNanoseconds << "ns,"
        //          << " actually waited " << actualWaitTimeNs << "ns, error is "
        //          << timeErrorPercent << "%\n";
        // std::cout << ss.str();
    }
    const auto averageErrorNs = static_cast<double>(totalAbsErrorNs) / trialCount;
    const auto averageErrorPercent = 100.0 * averageErrorNs / waitNanoseconds; {
        // std::stringstream ss;
        // ss << std::setprecision(2); // << std::fixed;
        // ss << "Average absolute error: " << averageErrorNs << "ns, "
        //         << averageErrorPercent << "%\n";
        // std::cout << ss.str();
    }
    return averageErrorPercent;
}

// Contains measurements of the errors observed in an experiment.
struct Results {
    long trialCount;
    long minimumNanoseconds;
    long maximumNanoseconds;
    long p25Nanoseconds;
    long p50Nanoseconds;
    long p75Nanoseconds;
};

Results characterizeWaiterAtFixedWait(timetools::TimerFactory &factory,
                                      const long waitNanoseconds,
                                      const long trialCount) {
    assertSuccess([]() { return timetools::setThisThreadAffinity(3); });
    assertSuccess([]() { return timetools::setThisThreadFifoRealtimePriority(95); });
    auto waiter = factory.createWaiter();
    long errors[trialCount];
    auto stopwatch = factory.createStopwatch();
    for (long trial = 1; trial <= trialCount; ++trial) {
        stopwatch.reset();
        stopwatch.start();
        waiter.busyWait(waitNanoseconds);
        stopwatch.stop();
        const auto actualWaitTimeNs = static_cast<int64_t>(stopwatch.getElapsedNanoseconds());
        const auto trialErrorNs = actualWaitTimeNs - waitNanoseconds;
        errors[trial - 1] = trialErrorNs;
    }
    std::sort(errors, errors + trialCount);
    Results results;
    results.trialCount = trialCount;
    results.minimumNanoseconds = errors[0];
    results.maximumNanoseconds = errors[trialCount - 1];
    results.p25Nanoseconds = errors[static_cast<long>(0.25 * trialCount)];
    results.p50Nanoseconds = errors[static_cast<long>(0.5 * trialCount)];
    results.p75Nanoseconds = errors[static_cast<long>(0.75 * trialCount)];
    return results;
}

// Sweeps the wait duration from the given minimum to the given maximum.
// Does one trial at each duration.
// Returns the actual measured wait time for each trial (in the order of the trials).
std::vector<long> characterizeWaiterOverWaitRange(timetools::TimerFactory &factory,
                                                  const long minimumWaitNs,
                                                  const long maximumWaitNs,
                                                  const long stepNs = 1) {
    if (maximumWaitNs < minimumWaitNs) {
        throw std::invalid_argument("Maximum must be not be less than minimum");
    }
    if (stepNs < 1) {
        throw std::invalid_argument("Step size must be at least 1");
    }
    assertSuccess([]() { return timetools::setThisThreadAffinity(3); });
    assertSuccess([]() { return timetools::setThisThreadFifoRealtimePriority(95); });
    std::vector<long> results;
    results.reserve((maximumWaitNs - minimumWaitNs) / stepNs + 1);
    auto waiter = factory.createWaiter();
    auto stopwatch = factory.createStopwatch();
    std::cout << "Starting sweep\n";
    for (long durationNs = minimumWaitNs; durationNs <= maximumWaitNs; durationNs += stepNs) {
        stopwatch.reset();
        stopwatch.start();
        waiter.busyWait(durationNs);
        stopwatch.stop();
        const auto actualWaitTimeNs = static_cast<int64_t>(stopwatch.getElapsedNanoseconds());
        results.push_back(actualWaitTimeNs);
    }
    return results;
}


TEST(Basic, TestBasic) {
    timetools::TimerFactory factory;
    getAverageErrorPercent(factory, 1000, 1000);
}

TEST(Benchmark, TestWaitAccuracy) {
    std::cout << std::unitbuf;
    timetools::TimerFactory factory;
    // {
    //     const auto ok = getAverageErrorPercent(factory, 100000, 1000);
    // }
    //return;

    constexpr auto GOAL_TEST_TIME_SECONDS = 5;
    constexpr auto MINIMUM_TRIAL_COUNT = 10;
    constexpr auto MAXIMUM_TRIAL_COUNT = 10000;
    for (long trialDurationNs = 2; trialDurationNs <= 2000000000; trialDurationNs *= 10) {
        int64_t trialCount = (GOAL_TEST_TIME_SECONDS * 1000000000ll) / trialDurationNs;
        if (trialCount < MINIMUM_TRIAL_COUNT)
            trialCount = MINIMUM_TRIAL_COUNT;
        else if (trialCount > MAXIMUM_TRIAL_COUNT)
            trialCount = MAXIMUM_TRIAL_COUNT;
        std::cout << "Doing " << trialCount << " trials of " << trialDurationNs << "ns waits\n";
        const auto errorPercent = getAverageErrorPercent(factory,
                                                         trialDurationNs,
                                                         trialCount);
        std::cout << trialDurationNs << ", " << errorPercent << "%\n";
    }
}

TEST(Benchmark, DeepTest) {
    std::cout << std::unitbuf;
    timetools::TimerFactory factory;
    constexpr auto GOAL_TEST_TIME_SECONDS = 5;
    constexpr auto MINIMUM_TRIAL_COUNT = 10;
    constexpr auto MAXIMUM_TRIAL_COUNT = 10000;
    std::cout << "Intended wait (ns)\tTrials\tMinimum (ns)\tp25 (ns)\tp50 (ns)\tp75 (ns)\tMaximum (ns)\n";
    for (long trialDurationNs = 1; trialDurationNs <= 100000000; trialDurationNs *= 10) {
        int64_t trialCount = (GOAL_TEST_TIME_SECONDS * 1000000000ll) / trialDurationNs;
        if (trialCount < MINIMUM_TRIAL_COUNT)
            trialCount = MINIMUM_TRIAL_COUNT;
        else if (trialCount > MAXIMUM_TRIAL_COUNT)
            trialCount = MAXIMUM_TRIAL_COUNT;
        const auto results = characterizeWaiterAtFixedWait(factory,
                                                           trialDurationNs,
                                                           trialCount);
        std::cout << trialDurationNs << "\t"
                << trialCount << "\t"
                << results.minimumNanoseconds << "\t"
                << results.p25Nanoseconds << "\t"
                << results.p50Nanoseconds << "\t"
                << results.p75Nanoseconds << "\t"
                << results.maximumNanoseconds << "\n";
    }
}

TEST(Benchmark, SweepWait) {
    std::cout << std::unitbuf;
    timetools::TimerFactory factory;
    constexpr auto MINIMUM_WAIT_NS = 1;
    constexpr auto MAXIMUM_WAIT_NS = 1000;
    constexpr auto STEP_NS = 1;
    const auto results = characterizeWaiterOverWaitRange(factory,
                                                         MINIMUM_WAIT_NS,
                                                         MAXIMUM_WAIT_NS,
                                                         STEP_NS);
    std::cout << "Intended wait (ns)\tActual wait(ns)\n";
    for (long waitNs = MINIMUM_WAIT_NS; waitNs <= MAXIMUM_WAIT_NS; waitNs += STEP_NS) {
        std::cout << waitNs << "\t" << results[waitNs - MINIMUM_WAIT_NS] << "\n";;
    }
}


void __attribute__ ((__noinline__)) fastLoop(uint64_t counter) {
    while (counter-- > 0);
}

inline void __attribute__((always_inline)) delay_loop(uint64_t counter) {
    // taken from https://codebrowser.dev/linux/linux/arch/x86/um/delay.c.html#__delay
    auto loops = (unsigned long) counter;
    asm volatile(
        "	test %0,%0	\n"
        "	jz 3f		\n"
        "	jmp 1f		\n"
        ".align 16		\n"
        "1:	jmp 2f		\n"
        ".align 16		\n"
        "2:	dec %0		\n"
        "	jnz 2b		\n"
        "3:	dec %0		\n"
        : "+a" (loops)
        :
    );
}

uint64_t timeFastLoop(timetools::TimerFactory &factory, long iterations) {
    auto sw = factory.createStopwatch();
    const auto TRIALS = 100;
    std::vector<uint64_t> loopTimeNs;
    loopTimeNs.reserve(TRIALS);
    for (int trial = 1; trial <= TRIALS; ++trial) {
        sw.start();
        delay_loop(iterations);
        sw.stop();
        loopTimeNs.push_back(sw.getElapsedNanoseconds());
    }
    std::sort(loopTimeNs.begin(), loopTimeNs.end());
    return loopTimeNs[loopTimeNs.size() / 2];
}

TEST(Benchmark, TimeLoop) {
    assertSuccess([]() { return timetools::setThisThreadAffinity(3); });
    assertSuccess([]() { return timetools::setThisThreadFifoRealtimePriority(95); });
    timetools::TimerFactory factory;
    std::cout << "Iterations\tTime (ns)\n";
    for (int iterations = 0; iterations <= 1000000; iterations += 1000) {
        const auto time = timeFastLoop(factory, iterations);
        std::cout << iterations << "\t" << time << "\n";
    }
}

inline void __attribute__((always_inline)) delayInline(long iterations) {
    int64_t i = iterations;
    asm volatile(
        "test %0,%0	    \n"
        "jz done		\n"
        "jmp loop	    \n"
        ".align 16		\n"
        "loop: dec %0   \n"
        "jnz loop    	\n"
        "done:	nop     \n"
        : "+a" (i)
    );
}

uint64_t timeInlineDelay(timetools::TimerFactory &factory, long iterations) {
    auto sw = factory.createStopwatch();
    const auto TRIALS = 100;
    std::vector<uint64_t> loopTimeNs;
    loopTimeNs.reserve(TRIALS);
    for (int trial = 1; trial <= TRIALS; ++trial) {
        sw.start();
    //     asm volatile(
    //     "add $0, %%ax\n"
    //     : // no inputs
    //     : // no outputs
    //     : "cc" // clobbers flags
    // );
        sw.stop();
        loopTimeNs.push_back(sw.getElapsedNanoseconds());
    }
    std::sort(loopTimeNs.begin(), loopTimeNs.end());
    return loopTimeNs[loopTimeNs.size() / 2];
}

TEST(Benchmark, TimeInlineDelay) {
    assertSuccess([]() { return timetools::setThisThreadAffinity(3); });
    assertSuccess([]() { return timetools::setThisThreadFifoRealtimePriority(95); });
    timetools::TimerFactory factory;
    std::cout << "Iterations\tTime (ns)\n";
    for (int iterations = 0; iterations <= 1000000; iterations += 1000) {
        const auto time = timeInlineDelay(factory, iterations);
        std::cout << iterations << "\t" << time << "\n";
    }
}
