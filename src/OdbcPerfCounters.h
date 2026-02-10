#pragma once

// OdbcPerfCounters.h — Optional compile-time performance instrumentation
//
// Enable with: cmake -DODBC_PERF_COUNTERS=ON
// When enabled, atomic counters track key operations in the fetch/execute path.
// Query via driver-specific SQLGetConnectAttr info types (SQL_FB_PERF_*).
//
// When disabled (default), all macros expand to nothing — zero overhead.

#include <cstdint>

#ifdef ODBC_PERF_COUNTERS

#include <atomic>

namespace OdbcJdbcLibrary {

struct PerfCounters {
    std::atomic<uint64_t> fetchCalls{0};         // Total SQLFetch / SQLFetchScroll calls
    std::atomic<uint64_t> conversionCalls{0};    // Total conv*() dispatches
    std::atomic<uint64_t> fetchPathAllocs{0};    // Heap allocations in fetch path
    std::atomic<uint64_t> mutexAcquires{0};      // Mutex/SRWLOCK acquires
    std::atomic<uint64_t> wideToAnsiConvs{0};    // W→A string conversions
    std::atomic<uint64_t> executeCalls{0};        // Total SQLExecute / SQLExecDirect calls

    void reset() {
        fetchCalls.store(0, std::memory_order_relaxed);
        conversionCalls.store(0, std::memory_order_relaxed);
        fetchPathAllocs.store(0, std::memory_order_relaxed);
        mutexAcquires.store(0, std::memory_order_relaxed);
        wideToAnsiConvs.store(0, std::memory_order_relaxed);
        executeCalls.store(0, std::memory_order_relaxed);
    }
};

// Global instance — one per driver DLL
extern PerfCounters g_perfCounters;

#define PERF_INC(counter)  (::OdbcJdbcLibrary::g_perfCounters.counter.fetch_add(1, std::memory_order_relaxed))
#define PERF_ADD(counter, n) (::OdbcJdbcLibrary::g_perfCounters.counter.fetch_add(n, std::memory_order_relaxed))

} // namespace OdbcJdbcLibrary

// Driver-specific SQLGetConnectAttr info types for performance counters
#define SQL_FB_PERF_FETCH_CALLS       19001
#define SQL_FB_PERF_CONVERSION_CALLS  19002
#define SQL_FB_PERF_FETCH_ALLOCS      19003
#define SQL_FB_PERF_MUTEX_ACQUIRES    19004
#define SQL_FB_PERF_WIDE_TO_ANSI      19005
#define SQL_FB_PERF_EXECUTE_CALLS     19006
#define SQL_FB_PERF_RESET             19099

#else // !ODBC_PERF_COUNTERS

#define PERF_INC(counter)       ((void)0)
#define PERF_ADD(counter, n)    ((void)0)

#endif // ODBC_PERF_COUNTERS
