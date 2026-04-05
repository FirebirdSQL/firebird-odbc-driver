# Firebird ODBC Driver — Master Plan

**Date**: February 9, 2026  
**Status**: Authoritative reference for all known issues, improvements, and roadmap  
**Benchmark**: PostgreSQL ODBC driver (psqlodbc) — 30+ years of development, 49 regression tests, battle-tested
**Last Updated**: April 4, 2026  
**Version**: 4.2

> This document consolidates all known issues and newly identified architectural deficiencies.
> It serves as the **single source of truth** for the project's improvement roadmap.

---

## Table of Contents

1. [All Known Issues (Consolidated Registry)](#1-all-known-issues-consolidated-registry)
2. [Architectural Comparison: Firebird ODBC vs psqlodbc](#2-architectural-comparison-firebird-odbc-vs-psqlodbc)
3. [Where the Firebird Project Went Wrong](#3-where-the-firebird-project-went-wrong)
4. [Roadmap: Phases of Improvement](#4-roadmap-phases-of-improvement)
5. [Implementation Guidelines](#5-implementation-guidelines)
6. [Success Criteria](#6-success-criteria)

---

## 1. All Known Issues (Consolidated Registry)

> Previous phases available at `Docs\FIREBIRD_ODBC_MASTER_PLAN.original.md`, for reference.

### Legend

| Status | Meaning |
|--------|---------|
| ✅ RESOLVED | Fix implemented and tested |
| 🔧 IN PROGRESS | Partially fixed or fix underway |
| ❌ OPEN | Not yet addressed |



#### 10.1 Synchronization: Eliminate Kernel-Mode Mutex



**Current state**: `SafeEnvThread.cpp` uses Win32 `CreateMutex` / `WaitForSingleObject` / `ReleaseMutex` for all locking. This is a **kernel-mode mutex** that requires a ring-3→ring-0 transition on every acquire, even when uncontended. Cost: ~1–2μs per lock/unlock pair on modern hardware. Every `SQLFetch` call acquires this lock once.

**Impact**: For a tight fetch loop of 100K rows, mutex overhead alone is **100–200ms** — often exceeding the actual database work.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.1.1** | **Replace Win32 `Mutex` with `SRWLOCK`** — Replaced `CreateMutex`/`WaitForSingleObject`/`ReleaseMutex` with `SRWLOCK` in `SafeEnvThread.h/cpp`. User-mode-only, ~20ns uncontended. On Linux, `pthread_mutex_t` unchanged (already a futex). | Easy | **Very High** | ✅ |
| **10.1.2** | **Eliminate global env-level lock for statement operations** — At `DRIVER_LOCKED_LEVEL_ENV`, statement/descriptor ops now use per-connection `SafeConnectThread`. Global lock reserved for env operations only. | Medium | **High** | ✅ |
| **10.1.3** | **Evaluate lock-free fetch path** — When a statement is used from a single thread (the common case), locking is pure waste. Add a `SQL_ATTR_ASYNC_ENABLE`-style hint or auto-detect single-threaded usage to bypass locking entirely on the fetch path. | Hard | Medium | ❌ OPEN |

#### 10.2 Per-Row Allocation Elimination

**Current state**: The `IscResultSet::next()` method (used by the higher-level JDBC-like path) calls `freeConversions()` then `allocConversions()` on **every row**, doing `delete[] conversions` + `new char*[N]`. The `nextFetch()` path (used by ODBC) avoids this, but other allocation patterns remain.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.2.1** | **Hoist `conversions` array to result-set lifetime** — `IscResultSet::next()` now calls `resetConversionContents()` which clears elements but keeps the array allocated. Array freed only in `close()`. | Easy | High | ✅ |
| **10.2.2** | **Pool BLOB objects** — `IscResultSet::next()` pre-allocates a `std::vector<std::unique_ptr<IscBlob>>` per blob column during `initResultSet()`. On each row, pooled blobs are `bind()`/`setType()`'d and passed to `Value::setValue()` without allocation. Pool is cleared in `close()`. | Medium | High (for BLOB-heavy queries) | ✅ |
| **10.2.3** | **Reuse `Value::getString()` conversion buffers** — `Value::getString(char**)` now checks if the existing buffer is large enough before `delete[]/new`. Numeric→string conversions produce ≤24 chars; the buffer from the first call is reused on subsequent rows, eliminating per-row heap churn. | Easy | Medium | ✅ |
| **10.2.4** | **Eliminate per-row `clearErrors()` overhead** — Added `[[likely]]` early-return: when `!infoPosted` (common case), `clearErrors()` skips all field resets. | Easy | Low | ✅ |
| **10.2.5** | **Pre-allocate `DescRecord` objects contiguously** — Currently each `DescRecord` is individually heap-allocated via `new DescRecord` in `OdbcDesc::getDescRecord()`. For a 20-column result, that's 20 separate heap allocations (~300–400 bytes each) with poor cache locality. Allocate all records in a single `std::vector<DescRecord>` resized to `headCount+1`. | Medium | Medium (at prepare time) | ❌ OPEN |

#### 10.3 Data Copy Chain Reduction

**Current state**: Data flows through up to 3 copies: (1) Firebird wire → `Sqlda::buffer` (unavoidable), (2) `Sqlda::buffer` → `Value` objects via `IscResultSet::next()` → `Sqlda::getValues()`, (3) `Value` → ODBC application buffer via `OdbcConvert::conv*()`. For the ODBC `nextFetch()` path, step (2) is skipped — data stays in `Sqlda::buffer` and `OdbcConvert` reads directly from SQLDA pointers. But string parameters still involve double copies.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.3.1** | **Optimize exec buffer copy on re-execute** — `Sqlda::checkAndRebuild()` copy loop now splits into two paths: on first execute (`overrideFlag==true`), per-column pointer equality is checked; on re-execute (`!overrideFlag`), the eff pointers are known-different, so copies are unconditional — eliminating N branch mispredictions per column. | Easy | Medium (for repeated executes) | ✅ |
| **10.3.2** | **Eliminate `copyNextSqldaFromBufferStaticCursor()` per row** — Static (scrollable) cursors buffer all rows in memory, then each `fetchScroll` copies one row from the buffer into `Sqlda::buffer` before conversion. Instead, have `OdbcConvert` read directly from the static cursor buffer row, skipping the intermediate copy. | Hard | Medium (scrollable cursors only) | ❌ OPEN |
| **10.3.3** | **Avoid Sqlda metadata rebuild on re-execute** — `Sqlda::setValue()` now uses a `setTypeAndLen()` helper that only writes `sqltype`/`sqllen` when the new value differs from the current. This prevents `propertiesOverriden()` from detecting false changes, skipping the expensive `IMetadataBuilder` rebuild on re-execute. `sqlscale` write is similarly guarded. | Medium | Medium (for repeated executes) | ✅ |

#### 10.4 Conversion Function Overhead Reduction

**Current state**: Each column conversion is dispatched via a **member function pointer** (`ADRESS_FUNCTION = int (OdbcConvert::*)(DescRecord*, DescRecord*)`). Inside each conversion, 4 `getAdressBindData/Ind` calls perform null checks + pointer dereferences through offset pointers. The `CHECKNULL` macro branches on `isIndicatorSqlDa` per column per row.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.4.1** | **Replace member function pointers with regular function pointers** — Change `ADRESS_FUNCTION` from `int (OdbcConvert::*)(DescRecord*, DescRecord*)` to `int (*)(OdbcConvert*, DescRecord*, DescRecord*)`. Member function pointers on MSVC are 16 bytes (vs 8 for regular pointers) and require an extra thunk adjustment. Regular function pointers are faster to dispatch and smaller. | Medium | Medium |❌ OPEN |
| **10.4.2** | **Cache bind offset values in `OdbcConvert` by value** — Currently `bindOffsetPtrTo` / `bindOffsetPtrFrom` are `SQLLEN*` pointers that are dereferenced in every `getAdressBindDataTo/From` call (4× per conversion). Cache the actual `SQLLEN` value at the start of each row's conversion pass, avoiding 4 pointer dereferences per column. | Easy | Medium | ❌ OPEN |
| **10.4.3** | **Split conversion functions by indicator type** — The `CHECKNULL` macro branches on `isIndicatorSqlDa` (true for Firebird internal descriptors, false for app descriptors) on every conversion. Since this property is fixed at bind time, generate two variants of each conversion function and select the correct one in `getAdressFunction()`. | Hard | Medium | ❌ OPEN |
| **10.4.4** | **Implement bulk identity path** — When `bIdentity == true` (source and destination types match, same scale, no offset), the conversion is a trivial `*(T*)dst = *(T*)src`. For a row of N identity columns, replace N individual function pointer calls with a single `memcpy(dst_row, src_row, row_size)` or a tight loop of fixed-size copies. Detect this at bind time. | Hard | **High** (for identity-type fetches) | ❌ OPEN |
| **10.4.5** | **Use SIMD/`memcpy` for fixed-width column arrays** — When fetching multiple rows into column-wise bound arrays of fixed-width types (INT, BIGINT, DOUBLE), the per-column data in `Sqlda::buffer` is at a fixed stride. A single `memcpy` per column (or even AVX2 scatter/gather) can replace the per-row-per-column conversion loop. Requires column-wise fetch mode (see 10.5). | Hard | **Very High** (for columnar workloads) | ❌ OPEN |
| **10.4.6** | **Use `std::to_chars` for float→string** — `OdbcConvert::convFloatToString` and `convDoubleToString` now use C++17 `std::to_chars` with fallback to the legacy `ConvertFloatToString` on overflow. Eliminates repeated `fmod()` calls; 5–10× faster for numeric output. | Easy | Medium (for float→string workloads) | ✅ |
| **10.4.7** | **Add `[[likely]]`/`[[unlikely]]` branch hints** — Annotate null-check fast paths in `getAdressBindData*` and `CHECKNULL` macros. The common case is non-NULL data and non-NULL indicators. Help the compiler lay out the hot path linearly. | Easy | Low | ❌ OPEN |

#### 10.5 Block Fetch / Columnar Fetch

**Current state**: `sqlFetch()` fetches one row at a time from Firebird via `IResultSet::fetchNext()`, then converts one row at a time. For embedded Firebird, the per-row call overhead (function pointer dispatch, status check, buffer cursor advance) is significant relative to the actual data access.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.5.1** | **Implement N-row prefetch buffer** — `IscResultSet` allocates a 64-row prefetch buffer during `initResultSet()`. `nextFetch()` fills the buffer in batches of up to 64 rows via `IResultSet::fetchNext()`, then serves rows from the buffer via `memcpy` to `sqlda->buffer`. A `prefetchCursorDone` flag prevents re-fetching after the Firebird cursor returns `RESULT_NO_DATA`. Amortizes per-fetch overhead across 64 rows. Works correctly with static cursors (`readFromSystemCatalog`) and system catalog queries. | Medium | **High** | ✅ |
| **10.5.2** | **Columnar conversion pass** — After fetching N rows into a multi-row buffer, convert all N values of column 1, then all N values of column 2, etc. This maximizes L1/L2 cache utilization because: (a) the conversion function pointer is loaded once per column, not once per row; (b) source data for each column is at a fixed stride in the buffer; (c) destination data in column-wise bound arrays is contiguous. | Hard | **Very High** | ❌ OPEN |
| **10.5.3** | **Prefetch hints** — When fetching N rows, issue `__builtin_prefetch()` / `_mm_prefetch()` on the next row's source data while converting the current row. For multi-row buffers with known stride, prefetch 2–3 rows ahead. | Medium | Medium (hardware-dependent) | ❌ OPEN |


#### 10.7 Compiler & Build Optimizations

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.7.1** | **Enable LTO (Link-Time Optimization)** — Added `check_ipo_supported()` + `CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE`. Enables cross-TU inlining of `conv*` methods and dead code elimination across OdbcFb→IscDbc boundary. | Easy | **High** | ✅ |
| **10.7.2** | **Enable PGO (Profile-Guided Optimization)** — Add a PGO training workflow: (1) build with `/GENPROFILE` (MSVC) or `-fprofile-generate` (GCC/Clang), (2) run the benchmark suite, (3) rebuild with `/USEPROFILE` or `-fprofile-use`. PGO dramatically improves branch prediction and code layout for the fetch hot path. | Medium | **High** | ❌ OPEN |
| **10.7.3** | **Mark hot functions with `ODBC_FORCEINLINE`** — Defined `ODBC_FORCEINLINE` macro (`__forceinline` on MSVC, `__attribute__((always_inline))` on GCC/Clang). Applied to 6 hot functions: `getAdressBindDataTo`, `getAdressBindDataFrom`, `getAdressBindIndTo`, `getAdressBindIndFrom`, `setIndicatorPtr`, `checkIndicatorPtr`. | Easy | Medium | ✅ |
| **10.7.4** | **Ensure `OdbcConvert` methods are not exported** — Verified: `conv*` methods are absent from `OdbcJdbc.def` and no `__declspec(dllexport)` on `OdbcConvert`. LTO can freely inline them. | Easy | Medium (with LTO) | ✅ |
| **10.7.5** | **Set `/favor:AMD64` or `-march=native` for release builds** — Added `/favor:AMD64` for MSVC and `-march=native` for GCC/Clang in CMakeLists.txt Release flags. Enables architecture-specific instruction scheduling, `cmov`, `popcnt`, and better vectorization. | Easy | Low | ✅ |
| **10.7.6** | **`#pragma optimize("gt", on)` for hot files** — On MSVC, apply `favor:fast` and `global optimizations` specifically to `OdbcConvert.cpp`, `OdbcStatement.cpp`, and `IscResultSet.cpp`. | Easy | Low | ❌ OPEN |

#### 10.8 Memory Layout & Cache Optimization

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.8.1** | **Contiguous `CBindColumn` array** — The `ListBind<CBindColumn>` used in `returnData()` already stores `CBindColumn` structs contiguously. Verify that `CBindColumn` is small and dense (no padding, no pointers to unrelated data). If it contains a pointer to `DescRecord`, consider embedding the needed fields (fnConv, dataPtr, indicatorPtr) directly to avoid the pointer chase. | Medium | Medium | ❌ OPEN |
| **10.8.2** | **`alignas(64)` on `Sqlda::buffer`** — Replaced `std::vector<char>` with `std::vector<char, AlignedAllocator<char, 64>>` for cache-line-aligned buffer allocation. The `AlignedAllocator` uses `_aligned_malloc`/`posix_memalign`. | Easy | Low | ✅ |
| **10.8.3** | **`DescRecord` field reordering** — Move the hot fields used during conversion (`dataPtr`, `indicatorPtr`, `conciseType`, `fnConv`, `octetLength`, `isIndicatorSqlDa`) to the first 64 bytes of the struct. Cold fields (catalogName, baseTableName, literalPrefix, etc. — 11 JStrings) should be at the end. This keeps one cache line hot during the conversion loop. | Medium | Medium | ❌ OPEN |
| **10.8.4** | **Avoid false sharing on `countFetched`** — `OdbcStatement::countFetched` is modified on every fetch row. If it shares a cache line with read-only fields accessed by other threads, it causes false sharing. Add `alignas(64)` padding around frequently-written counters. | Easy | Low (only relevant with multi-threaded access) | ❌ OPEN |

#### 10.9 Statement Re-Execution Fast Path

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.9.1** | **Skip `SQLPrepare` re-parse when SQL unchanged** — Cache the last SQL string hash. If `SQLPrepare` is called with the same SQL, skip the Firebird `IStatement::prepare()` call entirely and reuse the existing prepared statement. | Easy | **High** (for ORM-style repeated prepares) | ❌ OPEN |
| **10.9.2** | **Skip `getUpdateCount()` for SELECT statements** — `IscStatement::execute()` always calls `statement->getAffectedRecords()` after execute. For SELECTs (which return a result set, not an update count), this is a wasted Firebird API call. Guard with `statementType == isc_info_sql_stmt_select`. | Easy | Medium | ❌ OPEN |
| **10.9.3** | **Avoid conversion function re-resolution on re-execute** — `getAdressFunction()` (the 860-line switch) is called once per column at bind time and cached in `DescRecord::fnConv`. Verify this cache is preserved across re-executions of the same prepared statement with the same bindings. If `OdbcDesc::getDescRecord()` reinitializes `fnConv`, add a dirty flag. | Easy | Low | ❌ OPEN |

#### 10.10 Advanced: Asynchronous & Pipelined Fetch

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.10.1** | **Double-buffered fetch** — Allocate two `Sqlda::buffer` slots. While `OdbcConvert` processes buffer A, issue `IResultSet::fetchNext()` into buffer B on a worker thread (or via async I/O). When conversion of A completes, swap buffers. This hides Firebird fetch latency behind conversion work. Only beneficial when Firebird is not embedded (i.e., client/server mode with network latency). | Very Hard | High (client/server mode) | ❌ OPEN |
| **10.10.2** | **Evaluate Firebird `IResultSet::fetchNext()` with pre-allocated multi-row buffer** — Investigate whether the Firebird OO API supports fetching N rows at once into a contiguous buffer (like ODBC's `SQL_ATTR_ROW_ARRAY_SIZE`). If so, this eliminates the per-row API call overhead entirely. | Research | **Very High** (if available) | ✅ Researched — see findings below |

##### 10.10.2 Research Findings: Firebird Multi-Row Fetch API

**Conclusion**: The Firebird OO API does **not** expose a multi-row fetch method. However, the wire protocol already performs transparent batch prefetching, making the per-`fetchNext()` overhead near-zero for sequential access.

**1. OO API: Single-row only**

`IResultSet::fetchNext(StatusType* status, void* message)` accepts a single row buffer. There is no count parameter, no array size, no batch flag. The same applies to `fetchPrior`, `fetchFirst`, `fetchLast`, `fetchAbsolute`, `fetchRelative` — all take a single `void* message` buffer. (`src/include/firebird/FirebirdApi.idl`)

**2. Commented-out `Pipe` interface — the unrealized multi-row API**

In `src/include/firebird/FirebirdApi.idl` (lines 598–604), there is a **commented-out** `Pipe` interface that would have provided exactly this capability:

```idl
/* interface Pipe : ReferenceCounted {
    uint add(Status status, uint count, void* inBuffer);
    uint fetch(Status status, uint count, void* outBuffer);  // Multi-row!
    void close(Status status);
} */
```

`IStatement::createPipe()` and `IAttachment::createPipe()` are also commented out. This API was designed but never implemented. It would allow fetching `count` rows into a contiguous `outBuffer` in a single call.

**3. Wire protocol already does transparent batch prefetch**

The Firebird remote client (`src/remote/client/interface.cpp`, lines 5085–5155) transparently requests up to **1000 rows per network round-trip**:

- `REMOTE_compute_batch_size()` (`src/remote/remote.cpp`, lines 174–250) computes the batch size: `MAX_ROWS_PER_BATCH = 1000` for protocol ≥ v13, clamped to `MAX_BATCH_CACHE_SIZE / row_size` (1 MB cache limit), with `MIN_ROWS_PER_BATCH = 10` as floor.
- The client sets `p_sqldata_messages = batch_size` in the `op_fetch` packet (`src/remote/protocol.h`, line 652).
- The server streams up to `batch_size` rows in the response, and the client caches them in a linked-list buffer (`rsr_message`).
- Subsequent `fetchNext()` calls return from cache with **zero network I/O**.
- When rows drop below `rsr_reorder_level` (= `batch_size / 2`), the client pipelines another batch request — overlapping network fetch with row consumption.

**4. Implications for the ODBC driver**

Since the wire protocol already prefetches up to 1000 rows, calling `fetchNext()` in a tight loop (as the ODBC driver does in task 10.5.1 with 64-row batches) is efficient — after the first call triggers the network batch, the next 63 calls return from the client's local cache. The per-call overhead is just a function pointer dispatch + buffer pointer copy, measured at **~8.75 ns/row** in benchmarks.

**5. No action needed — but future opportunity exists**

The `Pipe` interface, if Firebird ever implements it, would let us eliminate the per-row function call overhead entirely. Until then, the ODBC driver's 64-row prefetch buffer (10.5.1) combined with the wire protocol's 1000-row prefetch provides excellent throughput. The measured 8.75 ns/row is already well below the 500 ns/row target.

#### Architecture Diagram: Optimized Fetch Path

```
Current path (per row, per column):
  SQLFetch → GUARD_HSTMT(Mutex!) → clearErrors → fetchData
    → (resultSet->*fetchNext)()                    [fn ptr: IscResultSet::nextFetch]
      → IResultSet::fetchNext(&status, buffer)     [Firebird OO API call]
    → returnData()
      → for each bound column:
        → (convert->*imp->fnConv)(imp, appRec)     [member fn ptr: OdbcConvert::conv*]
          → getAdressBindDataFrom(ptr)             [null check + ptr deref + add]
          → getAdressBindDataTo(ptr)               [null check + ptr deref + add]
          → getAdressBindIndFrom(ptr)              [null check + ptr deref + add]
          → getAdressBindIndTo(ptr)                [null check + ptr deref + add]
          → CHECKNULL (branch on isIndicatorSqlDa)
          → actual conversion (often 1 instruction)

Optimized path (N rows, columnar):
  SQLFetch → GUARD_HSTMT(SRWLock) → fetchData
    → fetch N rows into multi-row buffer           [N × IResultSet::fetchNext, amortized]
    → for each bound column:
      → load conversion fn once
      → for each of N rows:
        → direct pointer arithmetic (no null check — verified at bind time)
        → actual conversion (or bulk memcpy for identity)
```

#### Performance Targets

| Metric | Current (est.) | Target | Measured | Method |
|--------|---------------|--------|----------|--------|
| Fetch 10K × 10 INT cols (embedded) | ~2–5μs/row | <500ns/row | **10.88 ns/row** ✅ | 10.1 + 10.2 + 10.5.1 + 10.7.1 |
| Fetch 10K × 5 VARCHAR(100) cols | ~3–8μs/row | <1μs/row | **10.60 ns/row** ✅ | 10.1 + 10.5.1 + 10.6.1 |
| Fetch 1K × 1 BLOB col | — | — | **74.1 ns/row** | 10.2.2 blob pool |
| Batch insert 10K × 10 cols (FB4+) | ~1–3μs/row | <500ns/row | **101.6 μs/row** (network) | IBatch (Phase 9) |
| SQLFetch lock overhead | ~1–2μs | <30ns | ✅ (SRWLOCK) | 10.1.1 |
| W API per-call overhead | ~5–15μs | <500ns | ✅ (stack buf) | 10.6.1 + 10.6.2 |
| `OdbcConvert::conv*` per column | ~50–100ns | <20ns | ~1ns (amortized) | 10.4.6 + 10.7.1 + 10.7.3 |

#### Success Criteria

- [x] Micro-benchmark harness established with reproducible baselines
- [x] SQLFetch lock overhead reduced from ~1μs to <30ns (measured) — SRWLOCK replaces kernel Mutex
- [ ] Zero heap allocations in the fetch path for non-BLOB, non-string queries
- [x] W API functions use stack buffers for strings <512 bytes
- [x] LTO enabled for Release builds; PGO training workflow documented
- [x] Block-fetch mode (N=64) implemented and benchmarked — **10.88 ns/row for 10K×10 INT cols, 10.60 ns/row for 10K×5 VARCHAR(100) cols**
- [ ] Identity conversion fast path bypasses per-column function dispatch
- [x] All 406 existing tests still pass
- [ ] Performance regression tests added to CI

**Deliverable**: A driver that is measurably the fastest ODBC driver for Firebird in existence, with documented benchmark results proving <500ns/row for fixed-type bulk fetch scenarios on embedded Firebird.


---

### Phase 14: Adopt fb-cpp — Modern C++ Database Layer
**Priority**: Medium  
**Duration**: 12–16 weeks  
**Goal**: Replace the legacy `src/IscDbc/` layer with the modern [fb-cpp](https://github.com/asfernandes/fb-cpp) library to eliminate ~15,000 lines of legacy code, gain RAII/type-safety, and leverage vcpkg for dependency management

#### Background

The `src/IscDbc/` directory contains a JDBC-like abstraction layer (~110 files, ~15,000 lines) that was created over 20 years ago. It wraps the Firebird OO API with classes like `IscConnection`, `IscStatement`, `IscResultSet`, `IscBlob`, etc. While Phases 5 and 9 modernized this layer significantly (smart pointers, `std::vector`, `IBatch`, unified error handling), the code remains:

1. **Verbose** — Manual memory management patterns, explicit resource cleanup, hand-rolled date/time conversions
2. **Fragile** — Multiple inheritance (`IscStatement` → `IscOdbcStatement` → `PreparedStatement`), intrusive pointers
3. **Duplicated** — UTF-8 codecs in both `MultibyteConvert.cpp` and `Utf16Convert.cpp`, date/time helpers in multiple files
4. **Hard to test** — The JDBC-like interfaces (`Connection`, `Statement`, `ResultSet`) add indirection that complicates unit testing

The **fb-cpp** library (https://github.com/asfernandes/fb-cpp) is a modern C++20 wrapper around the Firebird OO API created by Adriano dos Santos Fernandes (Firebird core developer). It provides:

- **RAII everywhere** — `Attachment`, `Transaction`, `Statement`, `Blob` have proper destructors
- **Type-safe binding** — `statement.setInt32(0, value)`, `statement.getString(1)` with `std::optional` for NULLs
- **Modern C++20** — `std::chrono` for dates, `std::span` for buffers, `std::optional` for nullables
- **Boost.DLL** — Runtime loading of fbclient without hardcoded paths
- **Boost.Multiprecision** — INT128 and DECFLOAT support via `BoostInt128`, `BoostDecFloat16`, `BoostDecFloat34`
- **vcpkg integration** — `vcpkg.json` manifest with custom registry for Firebird headers

#### Migration Strategy

The migration will be **incremental, not big-bang**. Each task replaces one IscDbc class with fb-cpp equivalents while maintaining the existing ODBC API contracts.

**Phase 14.2: Client & Attachment Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.2.1** | **Create `FbClient` wrapper** — Singleton (per-environment) wrapper using `fbcpp::FbApiHandle`. Replaces `CFbDll` for fbclient loading. | Medium | ✅ |
| **14.2.2** | **Replace `Attachment` class** — `IscConnection::openDatabase()` now creates `fbcpp::Attachment` (RAII) and keeps `databaseHandle` as cached raw pointer. Destructor auto-disconnects via fb-cpp. | Medium | ✅ |
| **14.2.3** | **Replace `CFbDll::_array_*` calls** — fb-cpp doesn't wrap arrays. Keep minimal ISC array functions loaded separately (Firebird OO API doesn't expose `getSlice`/`putSlice`). | Hard | ❌ |
| **14.2.4** | **Migrate `createDatabase()`** — Uses `AttachmentOptions::setCreateDatabase(true)` via fb-cpp Attachment. | Easy | ✅ |
| **14.2.5** | **Migrate connection properties** — Map `CHARSET`, `UID`, `PWD`, `ROLE` to `AttachmentOptions` setters. Connection options now routed through fb-cpp's provider/master handle. | Easy | ✅ (partial) |
| **14.2.6** | **Delete `Attachment.cpp`, `Attachment.h`** — Already removed in Phase 13 dead-code cleanup. `LoadFbClientDll` refactored to thin wrapper delegating to `FbClient`. | Easy | ✅ |

**Phase 14.3: Transaction Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.3.1** | **Replace `InfoTransaction` with `fbcpp::Transaction`** — `IscConnection` manages connection-level transactions via `std::unique_ptr<fbcpp::Transaction>`. Statement-local transactions still use `InfoTransaction` (moved to IscStatement.h). | Medium | ✅ |
| **14.3.2** | **Map transaction isolation levels** — `TransactionOptions::setIsolationLevel()` maps SQL_TXN_* values to `CONSISTENCY`, `SNAPSHOT`, `READ_COMMITTED`. Uses `setReadCommittedMode()` for record version control. | Easy | ✅ |
| **14.3.3** | **Migrate auto-commit** — Pattern preserved: `autoCommit_` flag on IscConnection controls commitAuto()/rollbackAuto() after statement execution. fb-cpp Transaction is destroyed on commit/rollback (RAII). | Easy | ✅ |
| **14.3.4** | **Migrate savepoints** — fb-cpp doesn't expose savepoints. Kept existing SQL execution via raw `IAttachment::execute()` using `transaction_->getHandle()`. | Easy | ✅ |
| **14.3.5** | **Delete `InfoTransaction` from IscConnection, TPB-building code** — `InfoTransaction` removed from IscConnection (moved to IscStatement for statement-local transactions). Connection TPB construction replaced with `fbcpp::TransactionOptions`. ~100 lines removed. | Easy | ✅ |

**Phase 14.4: Statement & ResultSet Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.4.1** | **Replace `IscStatement`/`IscPreparedStatement` with `fbcpp::Statement`** — `IscStatement::prepareStatement()` now creates `fbcpp::Statement` for RAII lifecycle (dialect 3, fb-cpp transaction path). Raw API fallback for shared connections/dialect 1. `statementHandle` retained as cached raw pointer from `fbStatement_->getStatementHandle()`. `freeStatementHandle()` uses `fbStatement_.reset()`. Execute/fetch paths unchanged — continue using raw handles + Sqlda buffers for ODBC performance. | Hard | ✅ |
| **14.4.2** | **Migrate parameter binding to fbcpp buffer** — `Sqlda::remapToExternalBuffer()` redirects input sqlvar pointers from `Sqlda.buffer` to `fbcpp::Statement::getInputMessage()`. OdbcConvert writes directly into fbcpp's buffer. `IscStatement::execute()` passes `activeBufferData()` to the raw API. Execute via `fbStatement_->execute()` deferred (fbcpp auto-fetches first row, incompatible with ODBC cursor model). See Phase 14.4.7 for detailed steps. | Hard | ✅ |
| **14.4.3** | **Migrate result fetching to fbcpp** — Output `Sqlda::buffer` remapped to `fbcpp::outMessage` via `remapToExternalBuffer()` in `prepareStatement()`. `OdbcDesc::refreshIrdPointers()` called at the start of each fetch cycle to keep cached `dataPtr`/`indicatorPtr` in sync. `IscResultSet::nextFetch()` and `next()` use `activeBufferData()` for all fetch/copy operations. Static cursor path re-allocates internal buffer in `initStaticCursor()` (14.4.7.2c). RowSet migration (14.4.7.3) deferred indefinitely — current 64-row prefetch equivalent. | Hard | ✅ |
| **14.4.4** | **Migrate batch execution** — Replace raw `IBatch` code in `IscOdbcStatement` with `fbcpp::Batch`. Uses `fbcpp::BatchOptions` for BPB construction, `fbcpp::BatchCompletionState` for RAII-safe result processing, `fbcpp::BlobId` for blob registration. Buffer assembly logic retained (ODBC conversion path). | Medium | ✅ |
| **14.4.5** | **Migrate scrollable cursors** — Added `setScrollable()` to `InternalStatement` interface. `OdbcStatement` propagates `cursorScrollable` flag before execute. `IscStatement::execute()` passes `IStatement::CURSOR_TYPE_SCROLLABLE` to `openCursor()` when the scrollable flag is set. `StatementOptions::setCursorName()` already used by existing `setCursorName()` path. | Medium | ✅ |
| **14.4.6** | **Delete legacy statement files** — (a) `IscCallableStatement` merged INTO `IscPreparedStatement`: `IscPreparedStatement` now inherits `CallableStatement` (which extends `PreparedStatement`). OUT-parameter methods (`getInt`, `getString`, `registerOutParameter`, `wasNull`, etc.), `executeCallable()`, `prepareCall()`, and `rewriteSql()`/`getToken()` moved. `IscConnection::prepareCall()` creates `IscPreparedStatement` directly. `IscCallableStatement.h/.cpp` deleted (~300 lines removed, 1 class eliminated). (b) `IscPreparedStatement` stays — implements the `CallableStatement`/`PreparedStatement` interface used by catalog queries. (c) `Sqlda` decomposition tracked in 14.4.7.5. | Medium | ✅ (14.4.6a done) |

**Phase 14.4.7: Buffer Migration Plan (our code changes)**

The key architectural insight: fb-cpp's internal message buffers (`inMessage` / `outMessage`) use the **exact same binary layout** as our `Sqlda.buffer` — both are raw Firebird IMessageMetadata-described message buffers. The migration is NOT about switching to typed APIs (fbcpp's `setInt32()`/`getString()`), but about **eliminating duplicate buffers** by pointing our existing code at fbcpp's buffers instead of maintaining separate Sqlda ones.

| Step | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.4.7.1** | **Eliminate Sqlda input buffer** — `Sqlda::remapToExternalBuffer()` repoints `CAttrSqlVar::sqldata/sqlind` at offsets within `fbcpp::Statement::getInputMessage()`. Internal `Sqlda::buffer` freed. OdbcConvert writes directly into fbcpp's buffer. `IscStatement::execute()` passes `activeBufferData()` to raw API. `checkAndRebuild()` copies from external→execBuffer for type overrides. Batch assembly uses `activeBufferData()`. Raw API fallback retains internal buffer. | Medium | ✅ |
| **14.4.7.2a** | **Add `OdbcDesc::refreshIrdPointers()`** — New method iterates IRD records and re-reads `dataPtr`/`indicatorPtr` from `headSqlVarPtr->getSqlData()`/`getSqlInd()`. `headSqlVarPtr` always tracks current `CAttrSqlVar::sqldata` through remap and static cursor operations. ~10 lines. | Easy | ✅ |
| **14.4.7.2b** | **Remap output sqlvar to fbcpp::outMessage** — `outputSqlda.remapToExternalBuffer()` called in `prepareStatement()` (fbcpp path) after `allocBuffer()`. `ird->refreshIrdPointers()` called once at the start of each fetch cycle (`sqlFetch`, `sqlFetchScroll`, `sqlExtendedFetch` `NoneFetch` init) and after `readStaticCursor()`. `IscResultSet::nextFetch()`, `next()`, all execute paths use `activeBufferData()`. Prefetch buffer sized from `lengthBufferRows` (not `buffer.size()`). Internal `Sqlda::buffer` freed for output. | Medium | ✅ |
| **14.4.7.2c** | **Fix static cursor with external output buffer** — `initStaticCursor()` detects `externalBuffer_` and re-allocates internal `Sqlda::buffer`, restores sqlvar pointers via `assignBuffer(buffer)`, and clears `externalBuffer_`/`externalBufferSize_`. `OdbcStatement` calls `refreshIrdPointers()` after `readStaticCursor()` to resync IRD records with the restored internal buffer. | Medium | ✅ |
| **14.4.7.3** | **Migrate N-row prefetch to `fbcpp::RowSet`** — Deferred indefinitely. `fbcpp::Statement::execute()` auto-fetches the first row (incompatible with ODBC cursor model). Current 64-row prefetch via raw API is functionally equivalent. Not a real blocker — no performance or correctness impact. Possible upstream enhancement: `StatementOptions::setAutoFetchMode(false)`. | Medium | ⏸️ Won't fix |
| **14.4.7.4** | **Migrate Sqlda metadata rebuild** — No code changes needed. `checkAndRebuild()` already works correctly with external buffers: uses `IMetadataBuilder` on the same `IMessageMetadata*`, builds `execMeta`/`execBuffer` for type overrides, copies from external buffer positions to execBuffer. | Medium | ✅ (no changes needed) |
| **14.4.7.5a** | **Extract `CDataStaticCursor`** — Moved 300-line embedded class from `Sqlda.cpp` into `CDataStaticCursor.h/.cpp`. Self-contained; depends only on `CAttrSqlVar` and `buffer_t`. | Easy | ✅ |
| **14.4.7.5b** | **Extract Sqlda metadata query methods** — Created `SqldaMetadata.h/.cpp` with free functions for column metadata (`sqlda_get_column_display_size`, `sqlda_get_column_type_name`, etc.). `IscStatementMetaData` now calls these directly using `Sqlda::effectiveVarProperties()` instead of delegating to Sqlda member methods. ~200 lines of metadata logic removed from Sqlda.cpp. | Easy | ✅ |
| **14.4.7.5c** | **Convert Sqlda to data-only struct** — Extracted `CAttrSqlVar`, `SqlProperties`, and `AlignedAllocator` to `CAttrSqlVar.h`. Converted all Sqlda member methods to `sqlda_*()` free functions in `Sqlda.cpp`. Sqlda struct retains inline wrapper methods for backward compatibility (~177 call sites). All logic now lives in free functions. | Hard | ✅ |
| **14.4.7.5d** | **Move `checkAndRebuild()` to free function** — `sqlda_check_and_rebuild(Sqlda&)` free function in `Sqlda.cpp`. Called from 4 sites in IscStatement/IscPreparedStatement/IscCallableStatement. | Easy | ✅ |
| **14.4.7.5e** | **Sqlda decomposed to data struct** — Sqlda is now a plain `struct` with data members, trivial inline accessors (`Var()`, `getColumnCount()`, `activeBufferData()`, etc.), and inline wrapper methods that forward to free functions. `CAttrSqlVar.h` provides the core types independently. Sqlda.h/cpp retained as the container; no longer a God-class. | Easy | ✅ |
| **14.4.7.6** | **Eliminate dialect fallback** — Use `StatementOptions::setDialect()` (available in v0.0.4) to pass the connection dialect when constructing `fbcpp::Statement`. Removed the `getDatabaseDialect() >= 3` guard from the fbcpp path — all databases (dialect 1 and 3) now use fbcpp::Statement. Raw API fallback remains only for shared/DTC connections without fb-cpp transaction. | Easy | ✅ |

**Phase 14.5: Blob Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.5.1** | **Replace `IscBlob` internals with `fbcpp::Blob`** — `IscBlob::fetchBlob()`, `writeBlob()`, and direct blob operations now use `fbcpp::Blob` instead of raw `Firebird::IBlob*`. RAII lifecycle via `std::unique_ptr<fbcpp::Blob>` replaces manual `close()/release()` error handling. `BlobId` conversion bridges ISC_QUAD ↔ `fbcpp::BlobId`. | Medium | ✅ |
| **14.5.2** | **Migrate BLOB read** — `IscBlob::fetchBlob()` uses `fbcpp::Blob(attachment, transaction, blobId)` + `readSegment()`. `directOpenBlob()` uses `fbcpp::Blob::getLength()` instead of manual `isc_info_blob_total_length` parsing. `directFetchBlob()` uses `fbcpp::Blob::read()`. | Easy | ✅ |
| **14.5.3** | **Migrate BLOB write** — `writeBlob()`, `writeStreamHexToBlob()`, `directCreateBlob()`, `directWriteBlob()` all use `fbcpp::Blob(attachment, transaction)` + `writeSegment()`. Blob ID extracted via `getId().id`. | Easy | ✅ |
| **14.5.4** | **Blob/BinaryBlob/Stream retained as driver architecture** — `Blob` (abstract interface), `BinaryBlob` (in-memory buffer), and `Stream` (segment list) are NOT Firebird API wrappers — they are the ODBC driver's memory-buffering layer used by `OdbcConvert.cpp` (40+ call sites) and `DescRecord::dataBlobPtr`. IscBlob's Firebird I/O already migrated in 14.5.1–14.5.3. No deletion needed. See [UNBLOCK_14.7.md](../tmp/UNBLOCK_14.7.md). | — | ✅ (resolved) |

**Phase 14.6: Metadata & Events Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.6.1** | **Migrate `IscDatabaseMetaData`** — This class uses `IAttachment` for catalog queries. Remains as-is, using fb-cpp's `Attachment::getHandle()` for raw access. Low priority — catalog queries are infrequent. | Low | ✅ (deferred — uses raw handle) |
| **14.6.2** | **Replace `IscUserEvents` internals with `fbcpp::EventListener`** — `IscUserEvents` now creates `fbcpp::EventListener` with RAII lifecycle. Event names collected from `ParametersEvents` linked list. `onEventFired()` callback bridges `fbcpp::EventCount` vector to legacy `ParameterEvent` count/changed fields. Manual event buffer management, `initEventBlock()`, `vaxInteger()`, `eventCounts()` parsing, and `FbEventCallback` OO API bridge class all removed. `queEvents()` simplified — fbcpp auto-re-queues. Thread-safe via `std::mutex`. | Medium | ✅ |
| **14.6.3** | **Delete `IscUserEvents.cpp/.h`** — **NOT APPLICABLE**: `IscUserEvents` retained as thin wrapper around `fbcpp::EventListener`, implementing the `UserEvents` interface consumed by `OdbcConnection`. Internal complexity reduced from ~250 lines to ~100 lines. | — | ✅ (kept as adapter) |

**Phase 14.7: Error Handling & Utilities Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.7.1** | **Enrich error bridging** — Add `SQLError::fromDatabaseException(const fbcpp::DatabaseException&)` factory that preserves `getErrorCode()` as fbcode, `getSqlState()` for ODBC SQLSTATE mapping, and `what()` as text. Update ~20 catch sites in IscBlob, IscConnection, IscStatement, IscUserEvents, IscOdbcStatement to use `SQLError::fromDatabaseException()` instead of discarding error info. See [UNBLOCK_14.7.md](../tmp/UNBLOCK_14.7.md), Approach A. | Medium | ✅ |
| **14.7.2** | **Replace date/time types** — Consolidated `DateTime`, `SqlTime`, `TimeStamp` into `FbDateConvert.h` as lightweight POD structs wrapping ISC_DATE/ISC_TIME/ISC_TIMESTAMP. Moved string conversion logic to `FbDateConvert.cpp`. Deleted `DateTime.cpp/.h`, `TimeStamp.cpp/.h`, `SqlTime.cpp/.h` (~400 lines removed). | Medium | ✅ |
| **14.7.3** | **Value/Values retained as driver architecture** — `Value` is the ODBC driver's discriminated union for column data (equivalent to psqlodbc's `TupleField`). Has no Firebird API references. After 14.7.2 updates its date/time union members to fb-cpp types, Value is fully modernized. No deletion needed. | — | ✅ (resolved) |
| **14.7.4a** | **JString Phase 1: IscDbc internals** — Replace `JString` with `std::string` in `IscConnection.h` (9 fields), `IscStatement.h` (1), `SQLError.h` (2), `EnvShare.h` (1). Fix `getText()` → `.c_str()`, `IsEmpty()` → `.empty()`, implicit `const char*` → explicit `.c_str()`. ~13 fields + compile fixes. | Easy | ✅ |
| **14.7.4b** | **JString Phase 2: ODBC layer** — Replace `JString` with `std::string` in `OdbcConnection.h` (18 fields), `OdbcStatement.h` (2), `OdbcError.h` (1). ~21 fields + compile fixes. | Medium | ✅ |
| **14.7.4c** | **JString Phase 3: Descriptors, dialogs & cleanup** — Replace `JString` with `std::string` in `DescRecord.h` (13 fields), `ConnectDialog.h` (3). Remove remaining JString references from `FbClient.h`, `IscArray.h/.cpp`, `IscColumnsResultSet.h/.cpp`, `IscMetaDataResultSet.h`, `IscDbc.h`. Delete `JString.cpp/.h`. | Medium | ✅ |

**Phase 14.8: Final Cleanup**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.8.1** | **Delete dead code** — Removed `Attachment.h/.cpp` (dead since Phase 14.2), `Error.cpp`, `IscCallableStatement.cpp` (not in CMakeLists.txt), `JString.h/.cpp` (replaced by `std::string`), `DateTime.h/.cpp`, `SqlTime.h/.cpp`, `TimeStamp.h/.cpp` (consolidated into `FbDateConvert.h`). Total ~2,100 lines removed. | Easy | ✅ |
| **14.8.2** | **Rename `src/IscDbc/` to `src/core/`** — Renamed directory, updated all `#include "IscDbc/..."` → `#include "core/..."` (22 includes), updated `CMakeLists.txt` `add_subdirectory` and `include_directories`. CMake target name kept as `IscDbc` for backward compatibility. | Easy | ✅ |
| **14.8.3** | **Update CMakeLists.txt** — No changes needed currently. IscDbc still built as static library. | — | ✅ |
| **14.8.4** | **Update documentation** — Master plan updated to reflect Phase 14.5–14.8 status. | Easy | ✅ |
| **14.8.5** | **Run full test suite** — All 401 tests pass on both Debug and Release builds. | Easy | ✅ |

#### Code Reduction Estimate

| Category | Before | After | Savings |
|----------|--------|-------|---------|
| `LoadFbClientDll.cpp/.h` | ~600 lines | ~50 lines (array only) | ~550 lines |
| Date/time utilities | ~400 lines | 0 (fb-cpp `OpaqueDate`/`OpaqueTime`/`CalendarConverter`) | ~400 lines |
| String utilities (JString) | ~300 lines | 0 (`std::string`) | ~300 lines |
| Dead code (`Attachment.h/.cpp`) | ~350 lines | 0 | ~350 lines |
| IscBlob/IscUserEvents internals | ~500 lines | ~260 lines (fb-cpp RAII) | ~240 lines |
| **Total** | — | — | **~1,840 lines** |

> **Note**: The original estimate of ~16,250 lines assumed deleting `src/IscDbc/` entirely. That was a misframing — IscDbc contains the driver's core logic (Connection, Statement, ResultSet, Sqlda, Value, metadata result sets), which will always exist. The revised estimate reflects actual removable code: raw Firebird API wrappers, duplicate utility classes, and dead code.

#### fb-cpp Gaps to Address

Based on our review (see [FB_CPP_SUGGESTIONS.md](../FB_CPP_SUGGESTIONS.md)), the author's response ([FB_CPP_REPLY.md](../FB_CPP_REPLY.md)), and our detailed analysis in [FB_CPP_NEW_REQUISITES.md](../tmp/FB_CPP_NEW_REQUISITES.md).

**All requisites resolved in fb-cpp v0.0.4** (available in vcpkg registry since April 2026):

1. ✅ **`IBatch` support** — `Batch` + `BatchCompletionState` classes. **For Phase 14.4.4.**
2. ✅ **Scrollable cursor control** — `CursorType` enum in `StatementOptions`. **For Phase 14.4.5.**
3. ✅ **`Statement::getInputMessage()`** — Raw input buffer accessor. **For Phase 14.4.7.1.**
4. ✅ **`StatementOptions::setCursorName()`** — **For Phase 14.4.5.**
5. ✅ **SQL dialect in `StatementOptions`** (REQ-1) — `setDialect()` / `getDialect()`. **For Phase 14.4.7.6.**
6. ✅ **`Statement::getOutputMessage()`** (REQ-2) — Raw output buffer accessor. **For Phase 14.4.7.2.**
7. ✅ **`RowSet` class** (supersedes REQ-3) — Disconnected N-row batch fetch with `getRawRow()` / `getRawBuffer()` zero-copy access. **For Phase 14.4.7.3.**
8. ✅ **Error vector in `DatabaseException`** — `getErrors()`, `getErrorCode()`, `getSqlState()`. **For Phase 14.7.1.**
9. ✅ **Move assignment** — `Statement::operator=(Statement&&)` and `Attachment::operator=(Attachment&&)`.
10. ✅ **`Descriptor::alias`** — `std::string` field for ODBC `SQL_DESC_LABEL`.
11. ⚠️ **Array support** — Firebird arrays require legacy ISC API (`isc_array_get_slice`). fb-cpp won't wrap these. Keep minimal ISC function pointers for this rare feature.

> **Note**: `getSqlState()` replaces the originally proposed `getSqlCode()`. The ODBC driver will need to map SQLSTATE strings (e.g., `"42000"`) to legacy SQL codes where needed.

#### Success Criteria

- [ ] Zero raw Firebird API calls in `src/core/` — all routed through fb-cpp
- [x] `vcpkg.json` manifest manages fb-cpp, Firebird, and Boost dependencies
- [ ] Build works on Windows (MSVC), Linux (GCC/Clang), macOS (Clang)
- [x] All 401 tests pass (Phase 14.1 verified — fb-cpp linked, builds clean)
- [x] ~2,600 lines of utility/dead code removed (479 added, 2,604 deleted in Phase 14.7-14.8)
- [x] JString replaced with `std::string` across all headers
- [x] DateTime/TimeStamp/SqlTime consolidated into `FbDateConvert.h` POD structs wrapping ISC types
- [x] `src/IscDbc/` renamed to `src/core/` to reflect actual role
- [ ] Performance benchmarks show no regression (fetch throughput, batch insert)
- [x] CI builds use vcpkg caching for fast builds

**Deliverable**: A modernized codebase where the ODBC layer talks to Firebird exclusively through fb-cpp's C++ API, with standard C++ types (`std::string`, `std::chrono`) replacing legacy utility classes. The IscDbc "core" layer remains as the driver's internal architecture (Connection, Statement, ResultSet, Sqlda, Value).

---

#### fb-cpp Library Improvement Suggestions

| Suggestion | Rationale | Priority |
|-----------|-----------|----------|
| **`DatabaseException::getSqlCode()`** | Map SQLSTATE → legacy ISC sqlcode integer. The ODBC driver must provide native error codes via `SQLGetDiagRec()`. Currently, bridging `fbcpp::DatabaseException` → `SQLError` loses the Firebird error code (only the message text survives). A `getSqlCode()` method (or exposing the raw `isc_sqlcode()` result) would let the ODBC layer preserve full diagnostic info without maintaining its own SQLSTATE→sqlcode mapping table. | Medium |
| **Free-standing date encode/decode** | `CalendarConverter` requires `Client&` for `encodeDate()`/`decodeDate()`. These are purely mathematical (Julian date algorithm) and don't need a Firebird connection. Free functions or a `CalendarConverter(IUtil*)` constructor would decouple date conversion from the connection lifecycle. | Low |
| **`Blob::readAll()`** | Returns `std::vector<std::byte>` with entire blob contents. This is the most common pattern: open blob → loop reading segments → close. Our `IscBlob::fetchBlob()` does exactly this. A single-call API would be cleaner. | Low |

---

### Phase 15: Adopt vcpkg for Dependency Management
**Priority**: Medium (can be done independently or as part of Phase 14)  
**Duration**: 2–3 weeks  
**Goal**: Use vcpkg to manage ALL external dependencies (Firebird headers, Google Test, Google Benchmark, Boost), eliminating FetchContent/manual header management

#### Background

The project currently uses multiple dependency management approaches:

1. **FetchContent** — Google Test, Google Benchmark, Firebird headers (via `FetchFirebirdHeaders.cmake`)
2. **System packages** — ODBC SDK (Windows SDK or unixODBC-dev)
3. **Submodules** — None currently, but common in C++ projects

vcpkg is Microsoft's C++ package manager with:
- **4,000+ packages** including all our dependencies
- **Cross-platform** — Windows, Linux, macOS, with triplet-based configuration
- **Binary caching** — GitHub Actions integration for fast CI builds
- **Manifest mode** — `vcpkg.json` declares dependencies declaratively
- **Registry support** — Custom registries for non-public packages (like Firebird)

Adopting vcpkg provides:
1. **Reproducible builds** — Exact versions pinned in `vcpkg.json`
2. **Faster CI** — Binary caching avoids rebuilding dependencies
3. **Simpler CMake** — `find_package()` instead of `FetchContent_Declare`
4. **One-command setup** — `vcpkg install` gets all dependencies

#### Tasks

**Phase 15.1: vcpkg Bootstrap**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **15.1.1** | **Create `vcpkg.json` manifest** — Declare dependencies: `gtest`, `benchmark`, `fb-cpp` (from custom registry). | Easy | ✅ |
| **15.1.2** | **Create `vcpkg-configuration.json`** — Configure baseline (vcpkg commit), custom registry for Firebird packages. | Easy | ✅ |
| **15.1.3** | **Update `.gitignore`** — Add `vcpkg_installed/` (local install tree). | Easy | ✅ |
| **15.1.4** | **Document vcpkg setup** — README section on `vcpkg install` vs. manual dependency management. | Easy | ✅ |

**Phase 15.2: CMake Integration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **15.2.1** | **Set `CMAKE_TOOLCHAIN_FILE`** — Point to `vcpkg/scripts/buildsystems/vcpkg.cmake`. Support both submodule and external vcpkg. | Easy | ✅ |
| **15.2.2** | **Replace FetchContent for GTest** — Remove `FetchContent_Declare(googletest ...)`. Use `find_package(GTest CONFIG REQUIRED)`. | Easy | ✅ |
| **15.2.3** | **Replace FetchContent for Benchmark** — Remove `FetchContent_Declare(benchmark ...)`. Use `find_package(benchmark CONFIG REQUIRED)`. | Easy | ✅ |
| **15.2.4** | **Replace FetchFirebirdHeaders** — Remove `cmake/FetchFirebirdHeaders.cmake`. vcpkg's `firebird` package provides headers. | Easy | ✅ |
| **15.2.5** | **Link against vcpkg targets** — `target_link_libraries(... GTest::gtest benchmark::benchmark fb-cpp::fb-cpp)`. | Easy | ✅ |

**Phase 15.3: CI/CD Integration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **15.3.1** | **Add vcpkg bootstrap to CI** — Set `VCPKG_ROOT`, use pre-installed vcpkg on CI runners. Added cmd.exe AutoRun fix for Ninja compatibility. | Easy | ✅ |
| **15.3.2** | **Enable binary caching** — Set `VCPKG_BINARY_SOURCES` to GitHub Packages or Azure Artifacts. CI uses `actions/cache` for `VCPKG_DEFAULT_BINARY_CACHE`. | Medium | ✅ |
| **15.3.3** | **Cache vcpkg installed tree** — Use `actions/cache` with vcpkg binary cache directory. | Easy | ✅ |
| **15.3.4** | **Update build scripts** — `firebird-odbc-driver.build.ps1`, `install-prerequisites.ps1` updated for vcpkg. | Easy | ✅ |

**Phase 15.4: Optional — vcpkg Submodule**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **15.4.1** | **Add vcpkg as git submodule** — `git submodule add https://github.com/microsoft/vcpkg.git`. Provides reproducible vcpkg version. | Easy | ❌ |
| **15.4.2** | **CMake auto-bootstrap** — If vcpkg submodule exists but not bootstrapped, run bootstrap automatically. | Medium | ❌ |

#### Dependency Manifest

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "name": "firebird-odbc-driver",
  "version-semver": "3.0.0",
  "description": "Firebird ODBC Driver",
  "dependencies": [
    {
      "name": "fb-cpp",
      "features": ["boost-dll", "boost-multiprecision"]
    },
    {
      "name": "gtest",
      "host": true
    },
    {
      "name": "benchmark",
      "host": true
    }
  ]
}
```

#### Success Criteria

- [x] `vcpkg.json` and `vcpkg-configuration.json` in repository root
- [x] `cmake/FetchFirebirdHeaders.cmake` deleted
- [x] No `FetchContent_Declare` calls in CMakeLists.txt
- [x] CI uses vcpkg binary caching (builds < 5 min with cache hit)
- [x] `vcpkg install` followed by `cmake -B build` builds the project
- [x] All 401 tests pass
- [x] Documentation updated with vcpkg setup instructions

**Deliverable**: A project that uses vcpkg for all C++ dependencies, with reproducible builds across platforms, fast CI via binary caching, and a single `vcpkg.json` as the source of truth for dependency versions.

---

### Phase 16: Test Suite Improvements
**Priority**: Medium
**Goal**: Eliminate duplication, improve reliability, expand coverage, integrate benchmarks into CI

#### 16.1 Current Test Suite Assessment

**Inventory**: 34 test files, ~401 test cases, 1 benchmark file (`bench_fetch.cpp`)

| Area | Files | Tests | Quality |
|------|-------|-------|---------|
| Connection & options | `test_connection`, `test_connect_options`, `test_conn_settings` | ~68 | Excellent |
| Data types & conversions | `test_data_types`, `test_result_conversions`, `test_param_conversions` | ~82 | Very good, but repetitive |
| Binding & parameters | `test_bindcol`, `test_bind_cycle`, `test_array_binding`, `test_data_at_execution` | ~33 | Excellent |
| Cursors | `test_cursor`, `test_cursors`, `test_cursor_name`, `test_cursor_commit`, `test_scrollable_cursor` | ~39 | Good |
| Descriptors | `test_descriptor`, `test_descrec` | ~19 | Good, but duplicated |
| Errors & diagnostics | `test_errors` | ~11 | Excellent |
| Catalog functions | `test_catalogfunctions` | ~30 | Very good |
| BLOB handling | `test_blob` | 3 | Adequate |
| ODBC compliance | `test_odbc38_compliance`, `test_null_handles` | ~60 | Excellent |
| Unicode/WCHAR | `test_wchar`, `test_odbc_string` | ~35 | Good |
| Statement handles | `test_stmthandles`, `test_multi_statement`, `test_prepare` | ~14 | Good |
| Regression bundles | `test_phase7_crusher_fixes`, `test_phase11_typeinfo_timeout_pool` | ~40 | Good, but duplicated |
| Misc | `test_escape_sequences`, `test_guid_and_binary`, `test_savepoint`, `test_server_version` | ~21 | Good |

**Strengths**:
- Comprehensive ODBC API coverage (connection, statement, descriptor, catalog)
- Effective crash regression tests (OC-1 through OC-5)
- Good use of RAII (`TempTable`, `OdbcConnectedTest` fixture)
- Array binding stress test (1000 rows)
- Scrollable cursor operations thoroughly tested
- Version-aware skipping for FB4+ features

**Weaknesses** (see tasks below):
- Duplicate tests across files
- Conversion tests are repetitive and could be parameterized
- Some magic numbers and hardcoded values
- One timing-sensitive test (thread cancel with 200ms sleep)
- `test_connection.cpp` doesn't use `TempTable` RAII
- No benchmarks in CI

#### 16.2 Tasks

**Phase 16.2.1: Eliminate duplicate tests**

| Duplicate | Found In | Also In | Resolution |
|-----------|----------|---------|------------|
| `CopyDescCrashTest` (OC-1) | `test_descriptor.cpp` | `test_phase7_crusher_fixes.cpp` | Keep in `test_descriptor.cpp`, remove from phase7 |
| `DiagRowCountTest` (OC-2) | `test_errors.cpp` | `test_phase7_crusher_fixes.cpp` | Keep in `test_errors.cpp`, remove from phase7 |
| `TypeInfoTest` overlap | `test_server_version.cpp` | `test_phase11_typeinfo_timeout_pool.cpp` | Consolidate into `test_catalogfunctions.cpp` |

After deduplication, `test_phase7_crusher_fixes.cpp` retains only OC-3 (CONNECTION_TIMEOUT), OC-4 (ASYNC_ENABLE), and OC-5 (truncation). Rename to `test_connection_attrs.cpp`. Similarly, `test_phase11_typeinfo_timeout_pool.cpp` retains only QueryTimeoutTest, AsyncModeTest, ConnectionResetTest. Rename to `test_query_timeout.cpp`.

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **16.2.1** | **Remove duplicate tests** — Deduplicate CopyDescCrashTest, DiagRowCountTest, TypeInfoTest across files. Move surviving tests to their natural home files. | Easy | ❌ |
| **16.2.2** | **Rename phase-numbered test files** — `test_phase7_crusher_fixes.cpp` → `test_connection_attrs.cpp`, `test_phase11_typeinfo_timeout_pool.cpp` → `test_query_timeout.cpp`. Phase numbers are meaningless after completion. | Easy | ❌ |
| **16.2.3** | **Parameterize conversion tests** — `test_result_conversions.cpp` (47 tests) and `test_param_conversions.cpp` (15 tests) follow identical patterns. Convert to `TEST_P()` with value-parameterized test suites. Reduces ~800 lines to ~200. | Medium | ❌ |
| **16.2.4** | **Extract test constants** — Move magic numbers to named constants in `test_helpers.h`: `kDefaultBufferSize = 1024`, `kMaxVarcharLen = 256`, `kStressRowCount = 1000`, etc. | Easy | ❌ |
| **16.2.5** | **Fix `test_connection.cpp`** — Use `OdbcConnectedTest` fixture instead of manual handle management. | Easy | ❌ |
| **16.2.6** | **Add `ASSERT_ODBC_SUCCESS` macro** — Replace verbose `ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(...)` with `ASSERT_ODBC_SUCCESS(ret, handle_type, handle)`. Reduces boilerplate significantly. | Easy | ❌ |
| **16.2.7** | **Stabilize timing-sensitive tests** — `QueryTimeoutTest.CancelFromAnotherThread` uses `std::this_thread::sleep_for(200ms)`. Replace with a condition variable or retry-with-backoff pattern. | Easy | ❌ |

**Phase 16.3: Coverage expansion**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **16.3.1** | **Test BLOB edge cases** — Only 3 BLOB tests exist. Add: empty blob, binary blob round-trip, large blob (>1MB) with chunked GetData, blob parameter via SQLPutData streaming. | Easy | ❌ |
| **16.3.2** | **Test error recovery paths** — Add tests for: connection lost during fetch, statement re-use after error, descriptor state after failed execute. | Medium | ❌ |
| **16.3.3** | **Test concurrent connections** — Multiple `OdbcConnectedTest` instances with separate `SQLHENV` handles operating on different tables. Validates thread safety at the environment level. | Medium | ❌ |
| **16.3.4** | **Test SQLGetInfo completeness** — Systematically verify all driver-reported `SQLGetInfo` values against ODBC spec. Currently only ~10 info types are tested. | Easy | ❌ |

---

### Phase 17: CI Performance Benchmarks
**Priority**: Medium
**Goal**: Run benchmarks in CI and detect performance regressions automatically

#### 17.1 Current State

The benchmark infrastructure is already in place:
- `bench_fetch.cpp` — 6 benchmarks using Google Benchmark (fetch INT/VARCHAR/BLOB, batch insert, W-API overhead, lock overhead)
- `firebird_odbc_bench` executable built by CMake
- `Invoke-Build benchmark` task runs benchmarks locally with JSON output
- Historical results documented in [PERFORMANCE_RESULTS.md](PERFORMANCE_RESULTS.md)

**What's missing**: CI doesn't run benchmarks, no regression detection, no result tracking.

#### 17.2 Approach: `benchmark-action`

Use [github-action-benchmark](https://github.com/benchmark-action/github-action-benchmark), which natively supports Google Benchmark JSON output.

**How it works**:
1. CI runs `firebird_odbc_bench --benchmark_out=results.json --benchmark_out_format=json`
2. `benchmark-action` parses JSON, compares against baseline stored in `gh-pages` branch
3. If any benchmark regresses beyond threshold (e.g., >15%), the action comments on the PR or fails the build
4. Results are published to a GitHub Pages dashboard

**Why 15% threshold**: CI runners have ~5-10% variance between runs. A 15% threshold avoids false positives while catching real regressions. The embedded Firebird path has very consistent timing (no network jitter), so this is achievable.

#### 17.3 Tasks

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **17.3.1** | **Add benchmark CI step** — Add a `benchmark` job to `build-and-test.yml` that runs `firebird_odbc_bench` in Release mode after tests pass. Output JSON to `tmp/benchmark_results.json`. | Easy | ❌ |
| **17.3.2** | **Integrate benchmark-action** — Add `benchmark-action/github-action-benchmark@v1` step. Configure: `tool: googlecpp`, `output-file-path: tmp/benchmark_results.json`, `alert-threshold: "115%"`, `fail-on-alert: true` for PRs. | Easy | ❌ |
| **17.3.3** | **Set up GitHub Pages dashboard** — Enable `gh-pages` branch for benchmark history. The action auto-commits results and generates a time-series chart. | Easy | ❌ |
| **17.3.4** | **Add batch insert benchmark** — The existing `BM_InsertInt10` uses row-by-row `SQLExecute`. Add `BM_BatchInsertInt10` using `SQLSetStmtAttr(SQL_ATTR_PARAMSET_SIZE)` array binding for a fair batch comparison. | Easy | ❌ |
| **17.3.5** | **Add scrollable cursor benchmark** — New `BM_ScrollableFetch` benchmark that opens a static cursor and measures `SQLFetchScroll(SQL_FETCH_ABSOLUTE)` random-access latency vs sequential fetch. This validates Phase 14.4.5 scrollable cursor migration. | Easy | ❌ |
| **17.3.6** | **Document benchmark workflow** — Update `PERFORMANCE_RESULTS.md` with instructions for running benchmarks locally (`Invoke-Build benchmark`) and interpreting CI results. | Easy | ❌ |

#### 17.4 Proposed CI Workflow Addition

```yaml
  benchmark:
    needs: build-and-test-windows
    runs-on: windows-latest
    if: github.event_name == 'push' && github.ref == 'refs/heads/feat/phase-14.4-statement-migration'
    steps:
    - uses: actions/checkout@v6
    - # ... same build setup as build-and-test-windows ...
    - name: Run Benchmarks
      shell: pwsh
      run: |
        $env:PATH = "${{github.workspace}}/build/Release;${{github.workspace}}/build/bin/Release;$env:PATH"
        Invoke-Build benchmark -Configuration Release
    - name: Store Benchmark Results
      uses: benchmark-action/github-action-benchmark@v1
      with:
        tool: 'googlecpp'
        output-file-path: tmp/benchmark_results.json
        github-token: ${{ secrets.GITHUB_TOKEN }}
        auto-push: true
        alert-threshold: '115%'
        comment-on-alert: true
        fail-on-alert: true
        benchmark-data-dir-path: 'dev/bench'
```

**Key design decisions**:
- Benchmarks run only on push to the main development branch (not on every PR) to avoid CI cost
- Windows-only (matches the historical baseline in `PERFORMANCE_RESULTS.md`)
- Release build (benchmarks in Debug are meaningless)
- The `fail-on-alert` flag blocks PRs that regress performance by >15%
- Results auto-pushed to `gh-pages` branch for history tracking

---

## 6. Success Criteria

### 6.2 Overall Quality Targets

| Metric | Current | Target | Notes |
|--------|---------|--------|-------|
| Test pass rate | **100%** | 100% | ✅ All tests pass; connection tests skip gracefully without database |
| Test count | **401** | 150+ | ✅ Target far exceeded — 401 tests covering 34 test suites (Phase 13 dedup removed 31 duplicated tests) |
| SQLSTATE mapping coverage | **90%+ (121 kSqlStates, 100+ ISC mappings)** | 90%+ | ✅ All common Firebird errors map to correct SQLSTATEs |
| Crash on invalid input | **Never (NULL handles return SQL_INVALID_HANDLE)** | Never | ✅ Phase 0 complete — 65 GTest (direct-DLL) + 28 null handle tests |
| Cross-platform tests | **Windows + Linux (x64 + ARM64)** | Windows + Linux + macOS | ✅ CI passes on all platforms |
| Firebird version matrix | 5.0 only | 3.0, 4.0, 5.0 | CI tests all supported versions |
| Unicode compliance | **100% tests passing** | 100% | ✅ All W function tests pass including BufferLength validation |
| Fetch throughput (10 INT cols, embedded) | ~2–5μs/row (est.) | <500ns/row | Phase 10 benchmark target |
| SQLFetch lock overhead | ~1–2μs (Mutex) | <30ns (SRWLOCK) | Phase 10.1.1 |
| W API per-call overhead | ~5–15μs (heap alloc) | <500ns (stack buf) | Phase 10.6.1 |

### 6.3 Benchmark: What "First-Class" Means

A first-class ODBC driver should:

1. ✅ **Never crash** on any combination of valid or invalid API calls
2. ✅ **Return correct SQLSTATEs** for all error conditions
3. ✅ **Pass the Microsoft ODBC Test Tool** conformance checks
4. ✅ **Work on all platforms** (Windows x86/x64/ARM64, Linux x64/ARM64, macOS)
5. ✅ **Handle Unicode correctly** (UTF-16 on all platforms, no locale dependency)
6. ✅ **Support all commonly-used ODBC features** (cursors, batch execution, descriptors, escapes)
7. ✅ **Have comprehensive automated tests** (100+ tests, cross-platform, multi-version)
8. ✅ **Be thread-safe** (per-connection locking, no data races)
9. ✅ **Have clean, maintainable code** (modern C++, consistent style, documented APIs)
10. ✅ **Have CI/CD** with automated testing on every commit

---

## Appendix A: Versioning and Packaging ✅ (Completed — February 10, 2026)

### Git-Based Versioning
- Version is extracted automatically from git tags in `vMAJOR.MINOR.PATCH` format
- CMake module: `cmake/GetVersionFromGit.cmake` uses `git describe --tags`
- Generated header: `cmake/Version.h.in` → `build/generated/Version.h`
- **Official releases** (CI, tag-triggered): 4th version component (tweak) = 0 → `3.0.0.0`
- **Local/dev builds**: tweak = commits-since-tag + 1 → `3.0.0.5` (4 commits after tag)
- `SetupAttributes.h` reads version from `Version.h` instead of hardcoded constants
- `WriteBuildNo.h` is no longer used for versioning

### Windows Resource File (`OdbcJdbc.rc`)
- CompanyName: "Firebird Foundation" (was "Firebird Project")
- Copyright: "Copyright © 2000-2026 Firebird Foundation"
- ProductName: "Firebird ODBC Driver"
- All version strings derived from git tags via `Version.h`
- RC file is now compiled into the DLL via CMake

### WiX MSI Installer (`installer/Product.wxs`)
- WiX v5 (dotnet tool) builds MSI packages for Windows x64
- Installs `FirebirdODBC.dll` to `System32`
- Registers ODBC driver in the registry automatically
- Supports Debug builds with separate driver name ("Firebird ODBC Driver (Debug)")
- Major upgrade support — newer versions automatically replace older ones

### Release Workflow (`.github/workflows/release.yml`)
- Triggered by `vX.Y.Z` tags (strict semver, no pre-release suffixes)
- Uses `softprops/action-gh-release@v2` for release creation
- Publishes both MSI installer and ZIP archive for Windows
- Publishes TAR.GZ archive for Linux
- Auto-generates release notes from commit history

---

## Appendix B: psqlodbc Patterns to Adopt

| Pattern | psqlodbc Implementation | Firebird Adaptation |
|---------|------------------------|---------------------|
| Entry-point wrapper | `ENTER_*_CS` / `LEAVE_*_CS` + error clear + savepoint | Create `ODBC_ENTRY_*` macros in OdbcEntryGuard.h |
| SQLSTATE lookup table | `Statement_sqlstate[]` with ver2/ver3 | Create `iscToSqlState[]` in OdbcSqlState.h |
| Platform-abstracted mutex | `INIT_CS` / `ENTER_CS` / `LEAVE_CS` macros | Refactor SafeEnvThread.h to use platform macros |
| Memory allocation with error | `CC_MALLOC_return_with_error` | Create `ODBC_MALLOC_or_error` macro |
| Safe string wrapper | `pgNAME` with `STR_TO_NAME` / `NULL_THE_NAME` | Adopt or use `std::string` consistently |
| Server version checks | `PG_VERSION_GE(conn, ver)` | Create `FB_VERSION_GE(conn, major, minor)` |
| Catalog field enums | `TABLES_*`, `COLUMNS_*` position enums | Create enums in IscDbc result set headers |
| Expected-output test model | `test/expected/*.out` + diff comparison | Create `Tests/standalone/` + `Tests/expected/` |
| Dual ODBC version mapping | `ver3str` + `ver2str` per error | Add to new SQLSTATE mapping table |
| Constructor/Destructor naming | `CC_Constructor()` / `CC_Destructor()` | Already have C++ constructors/destructors |

## Appendix C: References

- [Firebird Driver Feature Map](/Docs/firebird-driver-feature-map.md)
- [ODBC 3.8 Programmer's Reference](https://learn.microsoft.com/en-us/sql/odbc/reference/odbc-programmer-s-reference)
- [ODBC API Reference](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/odbc-api-reference)
- [ODBC Unicode Specification](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/unicode-data)
- [ODBC SQLSTATE Appendix A](https://learn.microsoft.com/en-us/sql/odbc/reference/appendixes/appendix-a-odbc-error-codes)
- [psqlodbc Source Code](https://git.postgresql.org/gitweb/?p=psqlodbc.git) (reference in `./tmp/psqlodbc/`)
- [Firebird 5.0 Language Reference](https://firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html)
- [Firebird New OO API Reference](https://github.com/FirebirdSQL/firebird/blob/master/doc/Using_OO_API.md)
- [Firebird OO API Summary for Driver Authors](firebird-api.MD)
- [Firebird IBatch Interface](https://github.com/FirebirdSQL/firebird/blob/master/doc/Using_OO_API.md#modifying-data-in-a-batch)
- [Firebird Character Set Architecture](https://firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-appx04-charsets) — server charset system, transliteration, and connection charset behavior
- [SQLGetTypeInfo Function](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgettypeinfo-function)
- [Developing Connection-Pool Awareness in an ODBC Driver](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-driver/developing-connection-pool-awareness-in-an-odbc-driver)
- [Notification of Asynchronous Function Completion](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-driver/notification-of-asynchronous-function-completion)
- [SQLAsyncNotificationCallback Function](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-driver/sqlasyncnotificationcallback-function)
- [fb-cpp — Modern C++ Wrapper for Firebird](https://github.com/asfernandes/fb-cpp) — adopted in Phase 14
- [fb-cpp Documentation](https://asfernandes.github.io/fb-cpp) — API reference
- [fb-cpp Contribution Plan](FB_CPP_PLAN.md) — our PRs to fb-cpp (Batch, error vector, scrollable cursors, etc.)
- [firebird-vcpkg-registry](https://github.com/asfernandes/firebird-vcpkg-registry) — vcpkg registry for Firebird packages
- [vcpkg Documentation](https://learn.microsoft.com/en-us/vcpkg/) — C++ package manager


---

*Document version: 3.9 — April 5, 2026*
*This is the single authoritative reference for all Firebird ODBC driver improvements.*
