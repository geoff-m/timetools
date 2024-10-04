#include <gtest/gtest.h>

#include "../../lib/include/timetools.h"

#include <chrono>
#include <cstdint>
#include <sstream>
#include <iomanip>

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
    const auto averageErrorPercent = 100.0 * averageErrorNs / waitNanoseconds;
    {
        // std::stringstream ss;
        // ss << std::setprecision(2); // << std::fixed;
        // ss << "Average absolute error: " << averageErrorNs << "ns, "
        //         << averageErrorPercent << "%\n";
        // std::cout << ss.str();
    }
    return averageErrorPercent;
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
    for (long trialDurationNs = 1; trialDurationNs <= 1000000000; trialDurationNs *= 10) {
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
