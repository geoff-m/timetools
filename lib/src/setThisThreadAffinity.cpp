#include "timetools.h"
#include <pthread.h>

namespace timetools {
    int setThisThreadAffinity(const int cpu) {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(cpu, &set);
        const auto currentThread = pthread_self();
        int error = pthread_setaffinity_np(currentThread, sizeof(set), &set);
        if (error != 0) return error;
        error = pthread_getaffinity_np(currentThread, sizeof(set), &set);
        if (error != 0) return error;
        return !CPU_ISSET(cpu, &set);
    }

    int setThisThreadFifoRealtimePriority(const int priority) {
        sched_param param;
        param.sched_priority = priority;
        return pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    }
}
