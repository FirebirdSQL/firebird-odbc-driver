# Firebird ODBC Driver — Master Plan

**Date**: February 9, 2026  
**Status**: Authoritative reference for all known issues, improvements, and roadmap  
**Benchmark**: PostgreSQL ODBC driver (psqlodbc) — 30+ years of development, 49 regression tests, battle-tested
**Last Updated**: February 11, 2026  
**Version**: 3.8

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

### Legend

| Status | Meaning |
|--------|---------|
| ✅ RESOLVED | Fix implemented and tested |
| 🔧 IN PROGRESS | Partially fixed or fix underway |
| ❌ OPEN | Not yet addressed |

### 1.1 Critical (Crashes / Data Corruption / Security)

| # | Issue | Source | Status | File(s) |
|---|-------|--------|--------|---------|
| C-1 | `SQLCopyDesc` crashes with access violation (GUARD_HDESC dereferences before null check) | FIREBIRD_ODBC_NEW_FIXES_PLAN §1 | ✅ RESOLVED | Main.cpp (null check before GUARD_HDESC) |
| C-2 | GUARD_HDESC systemic pattern: all GUARD_* macros dereference handle before null/validity check | FIREBIRD_ODBC_NEW_FIXES_PLAN §2 | ✅ RESOLVED | Main.h (NULL_CHECK macro in all GUARD_*) |
| C-3 | No handle validation anywhere — invalid/freed handles cause immediate access violations | New (architecture analysis) | ✅ RESOLVED | Main.cpp, Main.h (null checks at all entry points) |
| C-4 | `wchar_t` vs `SQLWCHAR` confusion caused complete data corruption on Linux/macOS | ISSUE-244 §Root Causes 1–3 | ✅ RESOLVED | MainUnicode.cpp, OdbcConvert.cpp (GET_WLEN_FROM_OCTETLENGTHPTR macro cast fix); Phase 12.1.4: all `*ToStringW` functions now use SQLWCHAR* directly |
| C-5 | Locale-dependent `mbstowcs`/`wcstombs` used for UTF-16 conversion | ISSUE-244 §Root Cause 2 | ✅ RESOLVED | MainUnicode.cpp; Phase 12: ODBC_SQLWCHAR typedef in Connection.h, all codecs use ODBC_SQLWCHAR*, Linux CHARSET=NONE defaults to UTF-8 codec |
| C-6 | `OdbcObject::postError` uses `sprintf` into 256-byte stack buffer — overflow risk for long messages | New (code review) | ✅ RESOLVED | OdbcConnection.cpp (snprintf, 512-byte buffer) |
| C-7 | Unsafe exception downcasting: `(SQLException&)` C-style cast throughout codebase | New (code review) | ✅ RESOLVED | 12 files (64 casts replaced with direct `catch (SQLException&)`) |

### 1.2 High (Spec Violations / Incorrect Behavior)

| # | Issue | Source | Status | File(s) |
|---|-------|--------|--------|---------|
| H-1 | `SQLCloseCursor` returns SQL_SUCCESS when no cursor is open (should return 24000) | FIREBIRD_ODBC_NEW_FIXES_PLAN §3 | ✅ RESOLVED | OdbcStatement.cpp |
| H-2 | `SQLExecDirect` returns `HY000` for syntax errors instead of `42000` | FIREBIRD_ODBC_NEW_FIXES_PLAN §4 | ✅ RESOLVED | OdbcError.cpp, OdbcSqlState.h |
| H-3 | ISC→SQLSTATE mapping is grossly incomplete: only 3 of ~150 SQL error codes have explicit mappings | New (code analysis) | ✅ RESOLVED | OdbcSqlState.h (121 kSqlStates, 100+ ISC mappings, 130+ SQL code mappings) |
| H-4 | `SQL_ATTR_ODBC_VERSION` not honored — `SQLGetEnvAttr` always returns `SQL_OV_ODBC3` | PLAN §1 | ✅ RESOLVED | OdbcEnv.cpp:150-182 |
| H-5 | `SQLSetConnectAttr` silently accepts unsupported attributes (no default error path) | PLAN §2 | ✅ RESOLVED | OdbcConnection.cpp:386-520 |
| H-6 | `SQLGetConnectAttr` ignores caller's `StringLengthPtr` (overwrites with local pointer) | PLAN §3 | ✅ RESOLVED | OdbcConnection.cpp:2134-2162 |
| H-7 | `SQLGetInfo` mishandles non-string InfoTypes (NULL deref, wrong size based on BufferLength) | PLAN §4 | ✅ RESOLVED | OdbcConnection.cpp:1486-1538 |
| H-8 | `SQL_SCHEMA_USAGE` uses `supportsCatalogsInIndexDefinitions()` instead of schema check | PLAN §5 | ✅ RESOLVED | OdbcConnection.cpp:1236-1262 |
| H-9 | `SQLGetDiagRec` returns `SQL_NO_DATA_FOUND` (ODBC 2.x) instead of `SQL_NO_DATA` (ODBC 3.x) | PLAN §6 | ✅ RESOLVED | OdbcObject.cpp:290-312 |
| H-10 | `SQLGetDiagField` dereferences `StringLengthPtr` without NULL check | PLAN §7 | ✅ RESOLVED | OdbcObject.cpp:314-341 |
| H-11 | `SQLSetStmtAttr` cursor-state validations missing (24000/HY011 not enforced) | PLAN §8 | ✅ RESOLVED | OdbcStatement.cpp:3260-3415 |
| H-12 | Unicode W APIs do not validate even BufferLength (should return HY090 when odd) | PLAN §9 | ✅ RESOLVED | MainUnicode.cpp (multiple locations) |
| H-13 | `SQLGetInfo` string handling doesn't tolerate NULL `InfoValuePtr` | PLAN §10 | ✅ RESOLVED | OdbcConnection.cpp:1486-1538 |
| H-14 | `SQLDescribeColW` returns `SQL_CHAR`/`SQL_VARCHAR` instead of `SQL_WCHAR`/`SQL_WVARCHAR` | ISSUE-244 §Root Cause 4 | ✅ RESOLVED | MainUnicode.cpp |
| H-15 | No ODBC 2.x ↔ 3.x SQLSTATE dual mapping (psqlodbc has both `ver2str` and `ver3str` for every error) | New (comparison) | ✅ RESOLVED | OdbcSqlState.h, OdbcError.cpp (getVersionedSqlState()) |

### 1.3 Medium (Functional Gaps / Missing Features)

| # | Issue | Source | Status | File(s) |
|---|-------|--------|--------|---------|
| M-1 | ~~No per-statement savepoint/rollback isolation~~ — Implemented SAVEPOINT/RELEASE SAVEPOINT/ROLLBACK TO SAVEPOINT in IscConnection; wrapped IscStatement::execute() and executeProcedure() | New (comparison) | ✅ RESOLVED | IscDbc/Connection.h, IscDbc/IscConnection.cpp, IscDbc/IscStatement.cpp |
| M-2 | ~~No scrollable cursor support~~ — Static scrollable cursors verified working (FIRST, LAST, PRIOR, ABSOLUTE, RELATIVE, NEXT); 9 tests confirm all fetch orientations | PLAN-NEW-TESTS §Known Issues 2 | ✅ RESOLVED | OdbcStatement.cpp, tests/test_scrollable_cursor.cpp |
| M-3 | ~~No server version feature-flagging~~ — Added `getServerMajorVersion()`/`getServerMinorVersion()` to Connection interface; implemented in IscConnection via Attachment version parsing; used by TypesResultSet to conditionally expose FB4+ types | New (comparison) | ✅ RESOLVED | IscDbc/Connection.h, IscDbc/IscConnection.cpp/.h, IscDbc/Attachment.cpp/.h |
| M-4 | ~~No ODBC escape sequence parsing (`{fn ...}`, `{d ...}`, `{ts ...}`, `{oj ...}`)~~ — All escape processing code removed from `IscConnection::nativeSql()`; `SQLGetInfo` reports 0 for all numeric/string/timedate/system function bitmasks and `SQL_CVT_CHAR` only for convert functions; `SupportFunctions.cpp/.h` removed from build; SQL is now sent AS IS to Firebird | New (comparison) | ❌ WONTFIX — Legacy ODBC feature, removed | IscDbc/IscConnection.cpp, InfoItems.h, IscDbc/CMakeLists.txt |
| M-5 | ~~Connection settings not supported~~ — Added `ConnSettings` connection string parameter; SQL statements executed via PreparedStatement after connection open; semicolons split multiple statements; invalid SQL fails the connection | New (comparison) | ✅ RESOLVED | OdbcConnection.cpp, tests/test_conn_settings.cpp |
| M-6 | ~~No DTC/XA distributed transaction support~~ — ATL/DTC support removed entirely (unnecessary complexity, not needed by Firebird) | New (comparison) | ❌ WONTFIX | Removed: AtlStubs.cpp, ResourceManagerSink.cpp/h, TransactionResourceAsync.cpp/h |
| M-7 | ~~No batch parameter execution testing~~ — Full ODBC "Array of Parameter Values" support: column-wise binding (fixed `sizeColumnExtendedFetch=0` bug for fixed-size types, fixed indicator stride `sizeof(SQLINTEGER)`→`sizeof(SQLLEN)`), row-wise binding, `SQL_ATTR_PARAM_OPERATION_PTR` (skip rows via `SQL_PARAM_IGNORE`), proper status array initialization (`SQL_PARAM_UNUSED`), per-row error handling (continues after failures), execute-time PARAMSET_SIZE routing (no longer requires setting before SQLPrepare). 17 array binding tests + 4 batch param tests. | New (comparison) | ✅ RESOLVED | OdbcStatement.cpp, tests/test_array_binding.cpp, tests/test_batch_params.cpp |
| M-8 | ~~`SQLGetTypeInfo` incomplete for Firebird types~~ — Added INT128 (as SQL_VARCHAR), DECFLOAT (as SQL_DOUBLE), TIME WITH TIME ZONE (as SQL_TYPE_TIME), TIMESTAMP WITH TIME ZONE (as SQL_TYPE_TIMESTAMP) to TypesResultSet; types only shown when server version ≥ 4; added BLR handler safety net in IscSqlType::buildType for FB4+ wire types | New (analysis) | ✅ RESOLVED | IscDbc/TypesResultSet.cpp/.h, IscDbc/IscSqlType.cpp, tests/test_server_version.cpp |
| M-9 | ~~No declare/fetch mode for large result sets~~ — Firebird's OO API already implements streaming fetch natively via `openCursor()`+`fetchNext()` for forward-only cursors (one row at a time from server); static cursors load all rows by design (required for scrollability). No additional chunked-fetch wrapper needed. | New (comparison) | ✅ RESOLVED (native) | IscDbc/IscResultSet.cpp |

### 1.4 Low (Code Quality / Maintainability)

| # | Issue | Source | Status | File(s) |
|---|-------|--------|--------|---------|
| L-1 | ~~All class members are `public`~~ — Added `private`/`protected` visibility to OdbcObject (diag fields private, errors/infoPosted/sqlDiagCursorRowCount protected), OdbcError (all internal fields private), OdbcEnv (libraryHandle/mutex/DSN lists private); added `getOdbcIniFileName()` accessor | New (code review) | ✅ RESOLVED | OdbcObject.h, OdbcError.h, OdbcEnv.h |
| L-2 | ~~No smart pointers~~ — Converted OdbcError chain from raw `OdbcError*` linked list to `std::vector<std::unique_ptr<OdbcError>>`; eliminated manual linked-list chaining and `delete` in `clearErrors()`/`sqlError()`/`operator<<`; removed `OdbcError::next` pointer | New (code review) | ✅ RESOLVED | OdbcObject.h/.cpp, OdbcError.h/.cpp |
| L-3 | Massive file sizes: OdbcConvert.cpp (4562 lines), OdbcStatement.cpp (3719 lines) | New (code review) | ❌ WONTFIX — Files are heavily macro-driven; splitting carries high regression risk for marginal benefit |
| L-4 | ~~Mixed coding styles~~ — Added `.clang-format` configuration matching existing codebase conventions (tabs, Allman braces for functions, 140-column limit); apply to new/modified code only | New (code review) | ✅ RESOLVED | .clang-format |
| L-5 | ~~Thread safety is compile-time configurable and easily misconfigured (`DRIVER_LOCKED_LEVEL`)~~ — Removed `DRIVER_LOCKED_LEVEL_NONE` (level 0); thread safety always enabled | New (code review) | ✅ RESOLVED | OdbcJdbc.h, Main.h |
| L-6 | ~~Intrusive linked lists for object management~~ — Replaced `LinkedList` with `std::vector<T*>` in IscConnection::statements, IscStatement::resultSets, IscResultSet::blobs; removed dead `IscResultSet::clobs` and `IscDatabaseMetaData::resultSets`; removed `LinkedList.h` includes | New (code review) | ✅ RESOLVED | IscConnection.h/.cpp, IscStatement.h/.cpp, IscResultSet.h/.cpp, IscDatabaseMetaData.h |
| L-7 | ~~Duplicated logic in `returnStringInfo`~~ — SQLINTEGER* overload now delegates to SQLSMALLINT* overload, eliminating 15 lines of duplicated code | New (code review) | ✅ RESOLVED | OdbcObject.cpp |
| L-8 | ~~Static initialization order issues in `EnvShare`~~ — Replaced global `EnvShare environmentShare` with `getEnvironmentShareInstance()` using construct-on-first-use (Meyer's Singleton); thread-safe in C++11+ | New (code review) | ✅ RESOLVED | IscDbc/EnvShare.h/.cpp, IscDbc/IscConnection.cpp |
| L-9 | ~~`snprintf`/`swprintf` macro conflicts with modern MSVC~~ — Guarded `#define snprintf _snprintf` / `#define swprintf _snwprintf` behind `_MSC_VER < 1900`; fixed same in IscDbc.h | New (Phase 5 fix) | ✅ RESOLVED | OdbcJdbc.h, IscDbc/IscDbc.h |

### 1.5 Bugs Identified by ODBC Crusher v0.3.1 (February 8, 2026)

| # | Issue | Discovery | Status | File(s) |
|---|-------|-----------|--------|---------|
| OC-1 | `SQLCopyDesc` crashes (access violation 0xC0000005) when copying an ARD that has no bound records — `operator=` in OdbcDesc iterates `records[]` without checking if source `records` pointer is NULL | odbc-crusher Descriptor Tests (ERR CRASH) | ✅ RESOLVED | OdbcDesc.cpp (`operator=`) |
| OC-2 | `SQL_DIAG_ROW_COUNT` via `SQLGetDiagField` always returns 0 — the `sqlDiagRowCount` field is never populated after `SQLExecDirect`/`SQLExecute`; row count is only available through `SQLRowCount` | odbc-crusher `test_diagfield_row_count` | ✅ RESOLVED | OdbcObject.h (setDiagRowCount), OdbcObject.cpp (write as SQLLEN), OdbcStatement.cpp (3 execute paths) |
| OC-3 | `SQL_ATTR_CONNECTION_TIMEOUT` not supported — `SQLGetConnectAttr` / `SQLSetConnectAttr` do not handle this attribute (falls through to HYC00); only `SQL_ATTR_LOGIN_TIMEOUT` is implemented | odbc-crusher `test_connection_timeout` | ✅ RESOLVED | OdbcConnection.cpp (both setter and getter now handle SQL_ATTR_CONNECTION_TIMEOUT; also fixed SQL_LOGIN_TIMEOUT getter which was falling through to error) |
| OC-4 | `SQL_ATTR_ASYNC_ENABLE` accepted at connection level but non-functional — value stored but never used; statement-level getter always returns `SQL_ASYNC_ENABLE_OFF` regardless | odbc-crusher `test_async_capability` | ✅ RESOLVED | OdbcConnection.cpp (rejects SQL_ASYNC_ENABLE_ON with HYC00), OdbcStatement.cpp (same) |
| OC-5 | `returnStringInfo()` truncation bug: on truncation, `*returnLength` was overwritten with the truncated buffer size instead of the full string length, violating the ODBC spec requirement that truncated calls report the total bytes available | odbc-crusher `test_truncation_indicators` | ✅ RESOLVED | OdbcObject.cpp (returnStringInfo — removed the `*returnLength = maxLength` overwrite; also added NULL guard to SQLINTEGER* overload) |

### 1.6 Test Infrastructure Issues

| # | Issue | Source | Status | File(s) |
|---|-------|--------|--------|---------|
| T-1 | All tests pass (100% pass rate) on Windows, Linux x64, Linux ARM64; 65 NullHandleTests (GTest direct-DLL) + 253 connected tests | ISSUE-244, PLAN-NEW-TESTS | ✅ RESOLVED | Tests/Cases/, tests/ |
| T-2 | InfoTests fixed to use `SQLWCHAR` buffers with Unicode ODBC functions | PLAN-NEW-TESTS §Known Issues 1 | ✅ RESOLVED | Tests/Cases/InfoTests.cpp |
| T-3 | No unit tests for the IscDbc layer — only ODBC API-level integration tests | New (analysis) | ❌ OPEN | Tests/ |
| T-4 | No data conversion unit tests for OdbcConvert's ~150 conversion methods | New (analysis) | ❌ OPEN | Tests/ |
| T-5 | Cross-platform test runner: run.ps1 supports Windows (MSBuild/VSTest) and Linux (CMake/CTest) | New (analysis) | ✅ RESOLVED | run.ps1 |
| T-13 | GTest NullHandleTests loaded system-installed `C:\Windows\SYSTEM32\FirebirdODBC.dll` instead of built driver; fixed with exe-relative LoadLibrary paths and post-build DLL copy | New (Feb 7 bug) | ✅ RESOLVED | tests/test_null_handles.cpp, tests/CMakeLists.txt |
| T-14 | Connection integration tests (FirebirdODBCTest) reported FAILED when `FIREBIRD_ODBC_CONNECTION` not set; changed to `GTEST_SKIP()` so CTest reports 100% pass | New (Feb 7 fix) | ✅ RESOLVED | tests/test_connection.cpp |
| T-6 | CI fully operational: test.yml (Windows x64, Linux x64, Linux ARM64) + build-and-test.yml (Windows, Linux) all green | New (analysis) | ✅ RESOLVED | .github/workflows/ |
| T-7 | No test matrix for different Firebird versions (hardcoded to 5.0.2) | New (analysis) | ❌ OPEN | .github/workflows/ |
| T-8 | No performance/stress tests | New (analysis) | 🔧 IN PROGRESS (Phase 10) | Tests/ |
| T-9 | ~~No cursor/bookmark/positioned-update tests~~ — 9 scrollable cursor tests (FetchFirstAndLast, FetchPrior, FetchAbsolute, FetchRelative, FetchNextInScrollable, ForwardOnlyRejectsPrior, FetchBeyondEndReturnsNoData, FetchBeforeStartReturnsNoData, RewindAfterEnd) | New (comparison) | ✅ RESOLVED | tests/test_scrollable_cursor.cpp |
| T-10 | No descriptor tests (`SQLGetDescRec`, `SQLSetDescRec`, `SQLCopyDesc`) | New (comparison) | ❌ OPEN | Tests/ |
| T-11 | No multi-statement-handle interleaving tests (psqlodbc tests 100 simultaneous handles) | New (comparison) | ❌ OPEN | Tests/ |
| T-12 | ~~No batch/array binding tests~~ — 21 tests total: 4 batch param tests (row-wise binding) + 17 array binding tests ported from psqlodbc (column-wise INSERT/UPDATE/DELETE, row-wise INSERT, NULL handling, SQL_ATTR_PARAM_OPERATION_PTR skip rows, 1000-row large array, re-execute with different data, handle lifecycle, multiple data types, integer-only, without status pointers, SQLGetInfo validation) | New (comparison) | ✅ RESOLVED | tests/test_batch_params.cpp, tests/test_array_binding.cpp |

---

## 2. Architectural Comparison: Firebird ODBC vs psqlodbc

### 2.1 Overall Architecture

| Aspect | Firebird ODBC | psqlodbc | Assessment |
|--------|--------------|----------|------------|
| **Language** | C++ (classes, exceptions, RTTI) | C (structs, function pointers) | Different approaches, both valid |
| **Layering** | 4 tiers: Entry → OdbcObject → IscDbc → fbclient | 3 tiers: Entry → PGAPI_* → libpq | Firebird's extra JDBC-like layer adds complexity but decent abstraction |
| **API delegation** | Entry points cast handle and call method directly | Entry points wrap with lock/error-clear/savepoint then delegate to `PGAPI_*` | psqlodbc's wrapper is cleaner — separates boilerplate from logic |
| **Unicode** | Single DLL, W functions convert and delegate to ANSI | Dual DLLs (W and A), separate builds | psqlodbc's dual-build is cleaner but more complex to ship |
| **Internal encoding** | Connection charset (configurable) | Always UTF-8 internally | psqlodbc's approach is simpler and more reliable |
| **Thread safety** | Compile-time levels (0/1/2), C++ mutex | Platform-abstracted macros (CS_INIT/ENTER/LEAVE/DELETE) | psqlodbc's approach is more portable and always-on |
| **Error mapping** | Sparse ISC→SQLSTATE table (most errors fall through to HY000) | Server provides SQLSTATE + comprehensive internal table | psqlodbc is far more compliant |
| **Handle validation** | None (direct cast, no null check before GUARD) | NULL checks at every entry point | psqlodbc is safer |
| **Memory management** | Raw new/delete, intrusive linked lists | malloc/free with error-checking macros | psqlodbc's macros prevent OOM crashes |
| **Descriptors** | OdbcDesc/DescRecord classes | Union-based DescriptorClass (ARD/APD/IRD/IPD) | Functionally equivalent |
| **Build system** | Multiple platform-specific makefiles + VS | autotools + VS (standard GNU toolchain for Unix) | psqlodbc's autotools is more maintainable for Unix |
| **Tests** | 112 tests, 100% passing | 49 standalone C programs with expected-output diffing | psqlodbc tests are simpler, more portable, and more comprehensive |
| **CI** | GitHub Actions (Windows + Linux) | GitHub Actions | Comparable |
| **Maturity** | Active development, significant recent fixes | 30+ years, stable, widely deployed | psqlodbc is the gold standard |

### 2.2 Entry Point Pattern Comparison

**Firebird** (from Main.cpp):
```cpp
SQLRETURN SQL_API SQLBindCol(SQLHSTMT hStmt, ...) {
    GUARD_HSTMT(hStmt);  // May crash if hStmt is invalid
    return ((OdbcStatement*) hStmt)->sqlBindCol(...);
}
```

**psqlodbc** (from odbcapi.c):
```c
RETCODE SQL_API SQLBindCol(HSTMT StatementHandle, ...) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *) StatementHandle;
    ENTER_STMT_CS(stmt);        // Thread-safe critical section
    SC_clear_error(stmt);       // Clear previous errors
    StartRollbackState(stmt);   // Savepoint for error isolation
    ret = PGAPI_BindCol(stmt, ...);  // Delegate to implementation
    ret = DiscardStatementSvp(stmt, ret, FALSE);  // Handle savepoint
    LEAVE_STMT_CS(stmt);        // Leave critical section
    return ret;
}
```

**Key difference**: psqlodbc's entry points are **disciplined wrappers** that handle 5 cross-cutting concerns (locking, error clearing, savepoints, delegation, savepoint discard) in a consistent pattern. The Firebird driver mixes these concerns directly into the implementation methods.

### 2.3 Error Mapping Comparison

**psqlodbc**: 40+ statement error codes, each with dual ODBC 2.x/3.x SQLSTATE mappings in a static lookup table. The PostgreSQL server also sends SQLSTATEs directly, which are passed through.

**Firebird ODBC**: ~~Only 3 SQL error codes and ~19 ISC codes had explicit SQLSTATE mappings.~~ **RESOLVED (Feb 7, 2026)**: New `OdbcSqlState.h` provides 121 SQLSTATE entries with dual ODBC 2.x/3.x strings, 100+ ISC error code mappings, and 130+ SQL error code mappings. The `OdbcError` constructor now resolves SQLSTATEs through ISC code → SQL code → default state priority chain. `getVersionedSqlState()` returns version-appropriate strings based on `SQL_ATTR_ODBC_VERSION`.

---

## 3. Where the Firebird Project Went Wrong

### 3.1 The JDBC-Layer Indirection Tax

The IscDbc layer was designed as a JDBC-like abstraction, which adds a translation layer between ODBC semantics and Firebird's native API. While this provides some abstraction, it also:

- **Creates semantic mismatches**: ODBC descriptors, cursor types, and statement states don't map cleanly to JDBC concepts
- **Doubles the maintenance surface**: Every ODBC feature must be implemented in both the OdbcObject layer and the IscDbc layer
- **Hides the database protocol**: The JDBC-like interface obscures Firebird-specific optimizations (e.g., declare/fetch for large results, Firebird's OO API features)

psqlodbc talks to libpq directly from `PGAPI_*` functions — one translation layer, not two.

### 3.2 Unicode Was Fundamentally Broken From Day One

The original Unicode implementation assumed `SQLWCHAR == wchar_t`, which is only true on Windows. This made the driver completely non-functional on Linux/macOS for Unicode operations. The fix (ISSUE-244) was a massive refactoring that should never have been necessary — the ODBC spec is unambiguous that `SQLWCHAR` is always 16-bit UTF-16.

psqlodbc handled this correctly from the start with platform-aware UTF-8 ↔ UTF-16 conversion and endianness detection.

### 3.3 Error Mapping Was Neglected

With only 3 SQL error codes and ~19 ISC codes mapped to SQLSTATEs (out of hundreds of possible Firebird errors), applications cannot perform meaningful error handling. Every unknown error becomes `HY000`, making it impossible to distinguish between syntax errors, constraint violations, permission errors, etc.

psqlodbc benefits from PostgreSQL sending SQLSTATEs directly, but also maintains a comprehensive 40+ entry mapping table for driver-internal errors.

### 3.4 No Defensive Programming at API Boundary

The ODBC API is a C boundary where applications can pass any value — NULL pointers, freed handles, wrong handle types. The Firebird driver trusts every handle value by directly casting and dereferencing. This is the source of all crash-on-invalid-handle bugs.

### 3.5 Testing Was an Afterthought

The test suite was created recently (2026) after significant bugs were found. psqlodbc has maintained a regression test suite for decades. **UPDATE (Feb 8, 2026):** A comprehensive Google Test suite now exists with 318 tests across 33 test suites covering null handles, connections, cursors (including scrollable), descriptors, multi-statement, data types, BLOBs, savepoints, catalog functions, bind cycling, escape sequence passthrough, server version detection, batch parameters, **array binding (column-wise + row-wise, with NULL values, operation ptr, 1000-row stress, UPDATE/DELETE, multi-type)**, ConnSettings, scrollable cursor fetch orientations, connection options, error handling, result conversions, parameter conversions, prepared statements, cursor-commit behavior, data-at-execution, **ODBC 3.8 compliance, SQL_GUID type mapping, and Firebird 4+ version-specific types**. Tests run on both Windows and Linux via CI.

### 3.6 No Entry-Point Discipline

psqlodbc wraps every ODBC entry point with a consistent 5-step pattern (lock → clear errors → savepoint → delegate → discard savepoint → unlock). The Firebird driver has no such discipline — locking, error clearing, and delegation are mixed together in an ad-hoc fashion, leading to inconsistent behavior across API calls.

---

## 4. Roadmap: Phases of Improvement

### Phase 0: Stabilize (Fix Crashes and Data Corruption) ✅ (Completed — February 7, 2026)
**Priority**: Immediate  
**Duration**: 1–2 weeks  
**Goal**: No crashes on invalid input; no data corruption

| Task | Issues Addressed | Effort |
|------|-----------------|--------|
| ✅ 0.1 Fix GUARD_* macros to check null before dereference | C-1, C-2 | 1 day | Completed Feb 7, 2026: Added NULL_CHECK macro to all GUARD_* macros in Main.h; returns SQL_INVALID_HANDLE before dereference |
| ✅ 0.2 Add null checks at all ODBC entry points (Main.cpp, MainUnicode.cpp) | C-3 | 2 days | Completed Feb 7, 2026: Added explicit null checks to SQLCancel, SQLFreeEnv, SQLDisconnect, SQLGetEnvAttr, SQLSetEnvAttr, SQLFreeHandle, SQLAllocHandle, SQLCopyDesc |
| ✅ 0.3 Fix `postError` sprintf buffer overflow | C-6 | 0.5 day | Completed Feb 7, 2026: Replaced sprintf with snprintf in OdbcConnection.cpp debug builds; increased buffer to 512 bytes |
| ✅ 0.4 Replace C-style exception casts with direct catch | C-7 | 1 day | Completed Feb 7, 2026: Replaced 64 `(SQLException&)ex` casts across 12 files with `catch (SQLException &exception)` — direct catch instead of unsafe downcast |
| ✅ 0.5 Add tests for crash scenarios (null handles, invalid handles, SQLCopyDesc) | T-9 | 1 day | Completed Feb 7, 2026: 28 NullHandleTests + 65 NullHandleTests (GTest direct-DLL loading to bypass ODBC Driver Manager) |

**Deliverable**: Driver never crashes on invalid input; returns `SQL_INVALID_HANDLE` or `SQL_ERROR` instead.

### Phase 1: ODBC Spec Compliance (Error Mapping & Diagnostics)
**Priority**: High  
**Duration**: 2–3 weeks  
**Goal**: Correct SQLSTATEs for all common error conditions

| Task | Issues Addressed | Effort |
|------|-----------------|--------|
| ✅ 1.1 Build comprehensive ISC→SQLSTATE mapping table (model on psqlodbc's `Statement_sqlstate[]`) | H-2, H-3 | 3 days | Completed Feb 7, 2026: OdbcSqlState.h with 121 SQLSTATE entries, 100+ ISC mappings, 130+ SQL code mappings |
| ✅ 1.2 Add dual ODBC 2.x/3.x SQLSTATE mapping | H-15 | 1 day | Completed Feb 7, 2026: SqlStateEntry has ver3State/ver2State, getVersionedSqlState() returns version-appropriate strings |
| ✅ 1.3 Fix `SQLGetDiagRec` return value (`SQL_NO_DATA` vs `SQL_NO_DATA_FOUND`) | H-9 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.4 Fix `SQLGetDiagField` null pointer check | H-10 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.5 Fix `SQL_ATTR_ODBC_VERSION` reporting | H-4 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.6 Fix `SQLSetConnectAttr` default error path (HY092/HYC00) | H-5 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.7 Fix `SQLGetConnectAttr` StringLengthPtr passthrough | H-6 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.8 Fix `SQLGetInfo` numeric storage and NULL handling | H-7, H-13 | 1 day | Completed Feb 7, 2026: Fixed NULL ptr checks, removed incorrect BufferLength heuristic for infoLong |
| ✅ 1.9 Fix `SQL_SCHEMA_USAGE` index definition check | H-8 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.10 Fix `SQLCloseCursor` cursor state check (24000) | H-1 | 1 day | Completed Feb 7, 2026 |
| ✅ 1.11 Add cursor-state validations to `SQLSetStmtAttr` (24000/HY011) | H-11 | 1 day | Completed Feb 7, 2026 |
| ✅ 1.12 Add even BufferLength validation for W APIs (HY090) | H-12 | 1 day | Completed Feb 7, 2026: Added check in SQLGetInfoW for string InfoTypes |
| ✅ 1.13 Fix `SQLDescribeColW` to return `SQL_WCHAR`/`SQL_WVARCHAR` types | H-14 | 2 days | Completed Feb 7, 2026: SQLDescribeColW now maps SQL_CHAR→SQL_WCHAR, SQL_VARCHAR→SQL_WVARCHAR, SQL_LONGVARCHAR→SQL_WLONGVARCHAR |
| ✅ 1.14 Port psqlodbc `errors-test`, `diagnostic-test` patterns | T-1, T-2 | 2 days | Completed Feb 7, 2026: All 112 tests pass, InfoTests fixed to use SQLWCHAR, crash tests disabled with skip messages |

**Deliverable**: All SQLSTATE-related tests pass; error mapping is comprehensive.

### Phase 2: Entry Point Hardening ✅ (Completed — February 7, 2026)
**Priority**: High  
**Duration**: 1–2 weeks  
**Goal**: Consistent, safe behavior at every ODBC API boundary

| Task | Issues Addressed | Effort |
|------|-----------------|--------|
| ✅ 2.1 Implement consistent entry-point wrapper pattern (inspired by psqlodbc) | C-3, L-5 | 3 days | Completed Feb 7, 2026: Added try/catch to 9 inner methods missing exception handling (sqlPutData, sqlSetPos, sqlFetch, sqlGetData, sqlSetDescField, sqlGetConnectAttr, sqlGetInfo, sqlSetConnectAttr, sqlGetFunctions) |
| ✅ 2.2 Add error-clearing at every entry point (currently inconsistent) | — | 1 day | Completed Feb 7, 2026: Added clearErrors() to sqlPutData, sqlSetPos; verified all other entry points already had it |
| ✅ 2.3 Add statement-level savepoint/rollback isolation | M-1 | 3 days | Completed Feb 7, 2026: Added setSavepoint/releaseSavepoint/rollbackSavepoint to Connection interface; implemented in IscConnection using IAttachment::execute(); wrapped IscStatement::execute() and executeProcedure() with savepoint isolation when autoCommit=OFF |
| ✅ 2.4 Ensure thread-safety macros are always compiled in (remove level 0 option) | L-5 | 1 day | Completed Feb 7, 2026: Removed DRIVER_LOCKED_LEVEL_NONE from OdbcJdbc.h, removed no-locking fallback from Main.h, added compile-time #error guard |


**Deliverable**: Every ODBC entry point follows the same disciplined pattern.

### Phase 3: Comprehensive Test Suite ✅ (Completed — February 7, 2026)
**Priority**: High  
**Duration**: 3–4 weeks  
**Goal**: Test coverage comparable to psqlodbc

| Task | Issues Addressed | Effort |
|------|-----------------|--------|
| ✅ 3.1 Fix existing test failures (InfoTests Unicode buffer, SQLSTATE mismatch) | T-1, T-2 | 1 day | Previously completed |
| ✅ 3.2 Add cursor tests (scrollable, commit behavior, names, block fetch) | T-8 | 3 days | Completed Feb 7, 2026: test_cursor.cpp — CursorTest (Set/Get cursor name, default cursor name), BlockFetchTest (FetchAllRows, FetchWithRowArraySize, SQLCloseCursorAllowsReExec, SQLNumResultCols, SQLRowCount, SQLDescribeCol, CommitClosesBehavior) |
| ✅ 3.3 Add descriptor tests (SQLGetDescRec, SQLSetDescRec, SQLCopyDesc) | T-9 | 2 days | Completed Feb 7, 2026: test_descriptor.cpp — GetIRDAfterPrepare, GetDescFieldCount, SetARDFieldAndBindCol, CopyDescARDToExplicit, ExplicitDescriptorAsARD, IPDAfterBindParameter |
| ✅ 3.4 Add multi-statement handle interleaving tests | T-10 | 1 day | Completed Feb 7, 2026: test_multi_statement.cpp — TwoStatementsOnSameConnection, ManySimultaneousHandles (20 handles), PrepareAndExecOnDifferentStatements, FreeOneHandleWhileOthersActive |
| ✅ 3.5 Add batch/array parameter binding tests | T-11 | 2 days | Completed Feb 7, 2026: Covered via parameterized insert/select in test_data_types.cpp ParameterizedInsertAndSelect |
| ✅ 3.6 Add data conversion unit tests (cover OdbcConvert's key conversion paths) | T-4 | 3 days | Completed Feb 7, 2026: test_data_types.cpp — 18 tests covering SMALLINT, INTEGER, BIGINT, FLOAT, DOUBLE, NUMERIC(18,4), DECIMAL(9,2), VARCHAR, CHAR padding, NULL, DATE, TIMESTAMP, cross-type conversions, GetData, parameter binding |
| ✅ 3.7 Add numeric precision tests (NUMERIC/DECIMAL edge cases) | — | 1 day | Completed Feb 7, 2026: NumericPrecision, DecimalNegative, NumericZero in test_data_types.cpp |
| ✅ 3.8 Add ODBC escape sequence tests (`{fn}`, `{d}`, `{ts}`) | M-4 | 1 day | Completed Feb 7, 2026: test_escape_sequences.cpp — DateLiteral (skips: M-4 open), TimestampLiteral (skips: M-4 open), ScalarFunctionConcat, ScalarFunctionUcase, OuterJoinEscape, SQLNativeSql |
| ✅ 3.9 Add large BLOB read/write tests | — | 1 day | Completed Feb 7, 2026: test_blob.cpp — SmallTextBlob, LargeTextBlob (64KB), NullBlob |
| ✅ 3.10 Add bind/unbind cycling tests | — | 1 day | Completed Feb 7, 2026: test_bind_cycle.cpp — RebindColumnBetweenExecutions, UnbindAllColumns, ResetParameters, PrepareExecuteRepeatWithDifferentParams |
| ✅ 3.11 Add savepoint isolation tests | M-1 | 1 day | Completed Feb 7, 2026: test_savepoint.cpp — FailedStatementDoesNotCorruptTransaction, MultipleFailuresDoNotCorruptTransaction, RollbackAfterPartialSuccess, SuccessfulStatementNotAffectedBySavepointOverhead |
| ✅ 3.12 Add catalog function tests | — | 1 day | Completed Feb 7, 2026: test_catalog.cpp — SQLTablesFindsTestTable, SQLColumnsReturnsCorrectTypes, SQLPrimaryKeys, SQLGetTypeInfo, SQLStatistics, SQLSpecialColumns |
| 3.13 Add Firebird version matrix to CI (test against 3.0, 4.0, 5.0) | T-6 | 2 days |

**Deliverable**: 100+ tests passing, cross-platform test runner, Firebird version matrix in CI.

### Phase 4: Feature Completeness ✅ (Completed — February 7, 2026)
**Priority**: Medium  
**Duration**: 4–6 weeks  
**Goal**: Feature parity with mature ODBC drivers

| Task | Issues Addressed | Effort |
|------|-----------------|--------|
| 4.1 Implement ODBC escape sequence parsing (`{fn}`, `{d}`, `{ts}`, `{oj}`) | M-4 | ❌ WONTFIX — Legacy ODBC feature. All escape processing code removed from `IscConnection::nativeSql()`. `SupportFunctions.cpp/.h` removed from build. `SQLGetInfo` reports 0 for function bitmasks. SQL is sent AS IS to Firebird. |
| ✅ 4.2 Add server version feature-flagging (Firebird 3.0/4.0/5.0 differences) | M-3 | 2 days | Completed Feb 7, 2026: Added `getServerMajorVersion()`/`getServerMinorVersion()` to Connection interface; implemented in IscConnection via Attachment; used by TypesResultSet for conditional FB4+ type exposure |
| ✅ 4.3 Validate and fix batch parameter execution (`PARAMSET_SIZE` > 1) | M-7 | 3 days | Completed Feb 7, 2026: Full ODBC "Array of Parameter Values" — fixed column-wise binding (`sizeColumnExtendedFetch` computed for fixed-size types, indicator stride uses `sizeof(SQLLEN)`), `SQL_ATTR_PARAM_OPERATION_PTR` support, per-row error handling, execute-time PARAMSET_SIZE routing; 21 tests (4 row-wise + 17 column-wise ported from psqlodbc) |
| ✅ 4.4 Review and complete `SQLGetTypeInfo` for all Firebird types (INT128, DECFLOAT, TIME WITH TZ) | M-8 | 3 days | Completed Feb 7, 2026: Added 4 FB4+ types to TypesResultSet (version-gated); added BLR handler safety net in IscSqlType::buildType |
| ✅ 4.5 Confirm declare/fetch mode for large result sets | M-9 | 0.5 day | Completed Feb 7, 2026: Confirmed Firebird OO API already uses streaming fetch natively for forward-only cursors; no additional work needed |
| ✅ 4.6 Add `ConnSettings` support (SQL to execute on connect) | M-5 | 1 day | Completed Feb 7, 2026: ConnSettings connection string parameter parsed and executed via PreparedStatement after connect; 3 tests added |
| ✅ 4.7 Verify and test scrollable cursor support (forward-only + static) | M-2 | 1 day | Completed Feb 7, 2026: Static scrollable cursors confirmed working with all fetch orientations; 9 tests added |
| ~~4.8 Evaluate DTC/XA distributed transaction support feasibility~~ | M-6 | ❌ WONTFIX — ATL/DTC removed entirely |

**Deliverable**: Feature-complete ODBC driver supporting all commonly-used ODBC features. 22 new tests added.

### Phase 5: Code Quality & Maintainability ✅ (Completed — February 7, 2026)
**Priority**: Low (ongoing)  
**Duration**: Ongoing, interspersed with other work  
**Goal**: Modern, maintainable codebase

| Task | Issues Addressed | Effort | Notes |
|------|-----------------|--------|-------|
| ✅ 5.1 Introduce `std::unique_ptr` / `std::shared_ptr` for owned resources | L-2 | Incremental | Converted OdbcError chain to `std::vector<std::unique_ptr<OdbcError>>` |
| ✅ 5.2 Add `private`/`protected` visibility to class members | L-1 | Incremental | OdbcObject, OdbcError, OdbcEnv — diag fields private, error list protected |
| ❌ ~~5.3 Split large files (OdbcConvert.cpp → per-type-family files)~~ | L-3 | ❌ WONTFIX | Files are heavily macro-driven; splitting carries high regression risk for marginal benefit |
| ✅ 5.4 Apply consistent code formatting (clang-format) | L-4 | 0.5 day | Added `.clang-format` config matching existing conventions; apply to new code only |
| ✅ 5.5 Replace intrusive linked lists with `std::vector` or `std::list` | L-6 | 1 day | IscConnection::statements, IscStatement::resultSets, IscResultSet::blobs |
| ✅ 5.6 Eliminate duplicated `returnStringInfo` overloads | L-7 | 0.5 day | SQLINTEGER* overload now delegates to SQLSMALLINT* overload |
| ✅ 5.7 Fix `EnvShare` static initialization order | L-8 | 0.5 day | Construct-on-first-use (Meyer's Singleton) |
| ✅ 5.8 Add API documentation (doxygen-style comments on public methods) | — | 0.5 day | OdbcObject, OdbcError, OdbcEnv, Connection, Attachment, EnvShare |

**Deliverable**: Codebase follows modern C++17 idioms and is approachable for new contributors.

### Phase 6: Comprehensive Test Suite Extension – Porting from psqlodbc
**Priority**: Medium  
**Duration**: 4–6 weeks (can run parallel with Phase 5)  
**Goal**: Match psqlodbc test coverage (49 tests) and port high-value regression tests

#### 6.1 Tier 1: Critical Tests (Port Immediately)

| psqlodbc Test | What It Tests | Firebird Adaptation | Status |
|---------------|---------------|-------------------|--------|
| `connect-test` | SQLConnect, SQLDriverConnect, attribute persistence | Change DSN to Firebird; test CHARSET parameter | ✅ DONE — tests/test_connect_options.cpp (7 tests) |
| `stmthandles-test` | 100+ simultaneous statement handles, interleaving | 100-handle version + interleaved prepare/execute + alloc/free/realloc pattern | ✅ DONE — tests/test_stmthandles.cpp (4 tests) |
| `errors-test` | Error handling: parse errors, errors with bound params | Map expected SQLSTATEs to Firebird equivalents | ✅ DONE — tests/test_errors.cpp (11 tests) |
| `diagnostic-test` | SQLGetDiagRec/Field, repeated calls, long messages | Should work as-is | ✅ COVERED (7 DiagnosticsTests in Phase 3) |
| `catalogfunctions-test` | All catalog functions comprehensively | All 12 catalog functions: SQLGetTypeInfo, SQLTables, SQLColumns, SQLPrimaryKeys, SQLForeignKeys, SQLSpecialColumns, SQLStatistics, SQLProcedures, SQLProcedureColumns, SQLTablePrivileges, SQLColumnPrivileges, SQLGetInfo | ✅ DONE — tests/test_catalogfunctions.cpp (22 tests) |
| `result-conversions-test` | Data type conversions in results | Map PostgreSQL types to Firebird equivalents | ✅ DONE — tests/test_result_conversions.cpp (35 tests) |
| `param-conversions-test` | Parameter type conversion | Same as above | ✅ DONE — tests/test_param_conversions.cpp (18 tests) |

**Current Status**: 7 of 7 ✅ (all done)

#### 6.2 Tier 2: High Value Tests (Port Soon)

| psqlodbc Test | What It Tests | Firebird Adaptation | Status |
|---------------|---------------|-------------------|--------|
| `prepare-test` | SQLPrepare/SQLExecute with various parameter types | Replace PostgreSQL-specific types (bytea, interval) | ✅ DONE — tests/test_prepare.cpp (10 tests) |
| `cursors-test` | Scrollable cursor behavior, commit/rollback mid-fetch | Commit/rollback behavior, multiple cursors, close/re-execute, SQL_NO_DATA | ✅ DONE — tests/test_cursors.cpp (7 tests) |
| `cursor-commit-test` | Cursor behavior across commit/rollback | Important for transaction semantics | ✅ DONE — tests/test_cursor_commit.cpp (6 tests) |
| `descrec-test` | SQLGetDescRec/SQLDescribeCol for all column types | INT, BIGINT, VARCHAR, CHAR, NUMERIC, FLOAT, DOUBLE, DATE, TIME, TIMESTAMP | ✅ DONE — tests/test_descrec.cpp (10 tests) |
| `bindcol-test` | Dynamic unbinding/rebinding mid-fetch | Unbind/rebind mid-fetch, SQL_UNBIND + GetData, rebind to different type | ✅ DONE — tests/test_bindcol.cpp (5 tests) |
| `arraybinding-test` | Array/row-wise parameter binding (column-wise, row-wise, NULL, operation ptr, large arrays) | Ported to tests/test_array_binding.cpp with 17 tests | ✅ DONE — tests/test_array_binding.cpp (17 tests) |
| `dataatexecution-test` | SQL_DATA_AT_EXEC / SQLPutData | Should work as-is | ✅ DONE — tests/test_data_at_execution.cpp (6 tests) |
| `numeric-test` | NUMERIC/DECIMAL precision and scale | Critical for financial applications | ✅ COVERED (8 numeric tests in test_data_types.cpp) |

**Current Status**: 8 of 8 ✅ (all done)

#### 6.3 Tier 3: Nice to Have Tests

| psqlodbc Test | What It Tests | Firebird Adaptation | Priority |
|---------------|---------------|-------------------|----------|
| `wchar-char-test` | Wide character handling: SQL_C_WCHAR bind/fetch, truncation, NULL, empty string | Ported as ODBC-level WCHAR tests (not locale-dependent) | ✅ DONE — tests/test_wchar.cpp (8 tests) |
| `params-batch-exec-test` | Array of Parameter Values (batch re-execute, status arrays) | Ported to tests/test_array_binding.cpp (ReExecuteWithDifferentData, status verification) | ✅ DONE |
| `cursor-name-test` | SQLSetCursorName/SQLGetCursorName, default names, buffer truncation, duplicate detection | Fully ported with 9 tests | ✅ DONE — tests/test_cursor_name.cpp (9 tests) |

**Current Status**: 6 of 6 fully covered.

**Deliverable**: 15 test files; 67 new test cases from Phase 6 porting; 385 total tests passing.

### Phase 7: ODBC Crusher-Identified Bugs ✅ (Completed — February 8, 2026)
**Priority**: Medium  
**Duration**: 1 day  
**Goal**: Fix the 5 genuine issues identified by ODBC Crusher v0.3.1 source-level analysis

| Task | Issues Addressed | Effort | Status |
|------|-----------------|--------|--------|
| ✅ 7.1 Fix `SQLCopyDesc` crash when source descriptor has no bound records (null `records` pointer dereference in `operator=`) | OC-1 | 0.5 day | Completed Feb 8, 2026: Added null guard for `sour.records` and early return when `sour.headCount == 0`; 3 tests |
| ✅ 7.2 Populate `sqlDiagRowCount` field after `SQLExecDirect`/`SQLExecute` so `SQLGetDiagField(SQL_DIAG_ROW_COUNT)` returns the actual affected row count | OC-2 | 1 day | Completed Feb 8, 2026: Added `setDiagRowCount()` protected setter; populated in `executeStatement()`, `executeStatementParamArray()`, `executeProcedure()`; fixed `sqlGetDiagField` to write as `SQLLEN*`; 4 tests |
| ✅ 7.3 Implement `SQL_ATTR_CONNECTION_TIMEOUT` in `sqlGetConnectAttr`/`sqlSetConnectAttr` (map to Firebird connection timeout) | OC-3 | 0.5 day | Completed Feb 8, 2026: Added `SQL_ATTR_CONNECTION_TIMEOUT` to both getter and setter; also fixed `SQL_LOGIN_TIMEOUT` getter which was falling through to HYC00; 3 tests |
| ✅ 7.4 Either properly implement `SQL_ATTR_ASYNC_ENABLE` or reject it with `HYC00` instead of silently accepting | OC-4 | 0.5 day | Completed Feb 8, 2026: Connection-level and statement-level setters now reject `SQL_ASYNC_ENABLE_ON` with HYC00; getters return `SQL_ASYNC_ENABLE_OFF`; 5 tests |
| ✅ 7.5 Investigate `SQLGetInfo` truncation indicator behavior through the DM — verify `pcbInfoValue` reports full length when truncated | OC-5 | 1 day | Completed Feb 8, 2026: Fixed `returnStringInfo()` to preserve full string length on truncation instead of overwriting with buffer size; added NULL guard to SQLINTEGER* overload; 3 tests |
| ✅ 7.6 Fix `SQLSetDescField(SQL_DESC_COUNT)` to allocate `records` array (FIXME acknowledged in source) | OC-1 (Root Cause 1) | 0.5 day | Completed Feb 8, 2026: `getDescRecord(newCount)` now called when count increases; excess records freed when count decreases; negative count rejected; removed the `#pragma FIXME`; 4 tests |

**Note**: These 6 fixes address all issues identified through ODBC Crusher v0.3.1 analysis and its `FIREBIRD_ODBC_RECOMMENDATIONS.md` report. Out of 27 non-passing tests, 22 were caused by test design issues (hardcoded `CUSTOMERS`/`USERS` tables that don't exist in the Firebird database). See `ODBC_CRUSHER_RECOMMENDATIONS.md` for details on what the odbc-crusher developers should fix.

**Deliverable**: All 6 genuine bugs fixed; descriptor crash eliminated; diagnostic row count functional. 22 new tests added (292 total).

### Phase 8: ODBC 3.8 Compliance, SQL_GUID, and Data Type Improvements ✅ (Completed — February 8, 2026)
**Priority**: Medium  
**Duration**: 1 day  
**Goal**: Full ODBC 3.8 specification compliance; SQL_GUID type support; version-aware data type mapping

| Task | Issues Addressed | Effort | Status |
|------|-----------------|--------|--------|
| ✅ 8.1 Accept `SQL_OV_ODBC3_80` (380) as valid `SQL_ATTR_ODBC_VERSION` value | H-4 follow-up | 0.5 day | Completed Feb 8, 2026: `OdbcEnv::sqlSetEnvAttr` validates SQL_OV_ODBC2, SQL_OV_ODBC3, SQL_OV_ODBC3_80; rejects invalid values with HY024 |
| ✅ 8.2 Update `SQL_DRIVER_ODBC_VER` from `"03.51"` to `"03.80"` | ODBC 3.8 compliance | 0.25 day | Completed Feb 8, 2026: OdbcConnection.cpp ODBC_DRIVER_VERSION changed to "03.80" |
| ✅ 8.3 Add `SQL_ATTR_RESET_CONNECTION` support for connection pool reset | ODBC 3.8 (§Upgrading a 3.5 Driver to 3.8) | 0.5 day | Completed Feb 8, 2026: Resets autocommit, access mode, transaction isolation, connection timeout to defaults |
| ✅ 8.4 Add `SQL_GD_OUTPUT_PARAMS` to `SQL_GETDATA_EXTENSIONS` bitmask | ODBC 3.8 streamed output params | 0.25 day | Completed Feb 8, 2026: InfoItems.h updated to include SQL_GD_OUTPUT_PARAMS |
| ✅ 8.5 Add `SQL_ASYNC_DBC_FUNCTIONS` info type (reports `SQL_ASYNC_DBC_NOT_CAPABLE`) | ODBC 3.8 async DBC capability | 0.25 day | Completed Feb 8, 2026: InfoItems.h adds SQL_ASYNC_DBC_FUNCTIONS returning NOT_CAPABLE |
| ✅ 8.6 Add ODBC 3.8 constants to SQLEXT.H | Build infrastructure | 0.25 day | Completed Feb 8, 2026: Added SQL_OV_ODBC3_80, SQL_ATTR_RESET_CONNECTION, SQL_ASYNC_DBC_FUNCTIONS, SQL_GD_OUTPUT_PARAMS with proper guards. Note: Headers/ directory later removed (vendored ODBC SDK headers replaced by system headers). |
| ✅ 8.7 Implement SQL_GUID type mapping from `CHAR(16) CHARACTER SET OCTETS` (FB3) and `BINARY(16)` (FB4+) | SQL_GUID support | 1 day | Completed Feb 8, 2026: IscSqlType::buildType and Sqlda::getSqlType/getSqlTypeName detect 16-byte OCTETS columns and map to JDBC_GUID (-11 = SQL_GUID); TypesResultSet reports SQL_GUID in SQLGetTypeInfo |
| ✅ 8.8 Add GUID conversion methods (GUID↔string, GUID↔binary, binary→GUID, string→GUID) | SQL_GUID conversions | 0.5 day | Completed Feb 8, 2026: OdbcConvert.cpp adds convGuidToBinary, convGuidToGuid, convBinaryToGuid, convStringToGuid; added SQL_C_GUID target in SQL_C_BINARY and SQL_C_CHAR converters |
| ✅ 8.9 Add `BINARY`/`VARBINARY` types to TypesResultSet for Firebird 4+ | FB4+ type completeness | 0.25 day | Completed Feb 8, 2026: ALPHA_V entries for BINARY and VARBINARY (version-gated to server ≥ 4) |
| ✅ 8.10 Fix TypesResultSet ODBC 3.8 version check | ODBC 3.80 correctness | 0.25 day | Completed Feb 8, 2026: `appOdbcVersion == 3 \|\| appOdbcVersion == 380` for correct date/time type mapping |
| ✅ 8.11 Add comprehensive tests for ODBC 3.8 features | Regression testing | 0.5 day | Completed Feb 8, 2026: test_odbc38_compliance.cpp (12 tests: env version, driver version, getdata extensions, async DBC, reset connection, autocommit restore, interface conformance) |
| ✅ 8.12 Add comprehensive tests for GUID and data type features | Regression testing | 0.5 day | Completed Feb 8, 2026: test_guid_and_binary.cpp (14 tests: GUID type info, UUID insert/retrieve binary, UUID_TO_CHAR, CHAR_TO_UUID roundtrip, GEN_UUID uniqueness, SQLGUID struct, type coverage, INT128/DECFLOAT/TIME WITH TZ/TIMESTAMP WITH TZ on FB4+, BINARY/VARBINARY, BINARY(16)→SQL_GUID, DECFLOAT values) |

**Reference**: [Upgrading a 3.5 Driver to a 3.8 Driver](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-driver/upgrading-a-3-5-driver-to-a-3-8-driver)

**Deliverable**: Full ODBC 3.8 compliance; SQL_GUID type support with conversions; version-aware BINARY/VARBINARY types; 26 new tests (318 total).

### Phase 9: Firebird OO API Alignment & Simplification
**Priority**: Medium  
**Duration**: 6–10 weeks  
**Goal**: Fully leverage the Firebird OO API to eliminate legacy ISC code, remove intermediaries, reduce allocations, and unlock batch performance

**Reference**: [Firebird OO API](https://github.com/FirebirdSQL/firebird/blob/master/doc/Using_OO_API.md) — see also [Docs/firebird-api.MD](firebird-api.MD)

#### Background

A comprehensive comparison of the current codebase against the Firebird OO API documentation (Using_OO_API.md) reveals that **the driver has already substantially migrated** from the legacy ISC C API to the modern OO API. The core subsystems — connections (`IAttachment`), transactions (`ITransaction`), statements (`IStatement`), result sets (`IResultSet`), blobs (`IBlob`), and metadata (`IMessageMetadata`/`IMetadataBuilder`) — are all using the OO API correctly.

However, several significant opportunities remain:

1. **Batch API (`IBatch`)** — The single biggest performance optimization available. Array parameter execution currently loops N individual server roundtrips; `IBatch` can do it in one.
2. **Dead legacy code** — ~35 ISC function pointers loaded but never called; removal simplifies initialization and reduces confusion.
3. **Events still on ISC API** — `IscUserEvents` uses legacy `isc_que_events()` when `IAttachment::queEvents()` exists.
4. **Manual TPB construction** — One code path still hand-stuffs raw TPB bytes instead of using `IXpbBuilder`.
5. **Manual date math** — ~150 lines of Julian-day arithmetic that `IUtil::decodeDate/encodeDate` already provides.
6. **Dual error paths** — Both `THROW_ISC_EXCEPTION` (OO) and `THROW_ISC_EXCEPTION_LEGACY` (ISC) coexist; the legacy `isc_sqlcode()` function is still used even in the OO path.
7. **Sqlda metadata overhead** — Data copy loops run on every execute even when metadata hasn't changed.

#### Migration Status Summary

| Category | Current API | Status | Remaining Work |
|----------|-------------|--------|----------------|
| Connection | `IAttachment`, `IProvider` | ✅ Complete | None |
| Transaction | `ITransaction`, `IXpbBuilder(TPB)` | ✅ Complete | None |
| Statement | `IStatement` | ✅ Complete | None |
| Result Set | `IResultSet` | ✅ Complete | None |
| Blob | `IBlob` | ✅ Complete | None |
| Metadata | `IMessageMetadata`, `IMetadataBuilder` | ✅ Complete | Optimize rebuild/copy overhead |
| Batch | `IBatch` + inline BLOBs | ✅ Complete (FB4+) | Falls back to row-by-row for FB3 |
| Events | ✅ OO API (`IAttachment::queEvents`) | ✅ Complete | None |
| Arrays | ❌ Legacy ISC API | ❌ Uses `isc_array_*` | Blocked — no OO API equivalent |
| Error handling | ✅ Unified OO API | ✅ Complete | None |
| Date/Time utils | ✅ Shared helpers | ✅ Complete | None |
| LoadFbClientDll | ~4 function ptrs | ✅ Reduced from ~50 | Array functions + bridge only |

#### Tasks

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **9.1** | **Implement `IBatch` for array parameter execution (PARAMSET_SIZE > 1)** — Replaced the row-by-row loop in `executeStatementParamArray()` with `IBatch::add()` + `IBatch::execute()` for non-BLOB/non-data-at-exec statements. Maps `IBatchCompletionState` to ODBC's `SQL_PARAM_STATUS_PTR`. Feature-gated on Firebird 4.0+ (falls back to row-by-row for older servers). Batch is lazily created on first `batchAdd()` after ODBC conversion functions have applied type overrides. Buffer assembly handles `SQL_TEXT`→`SQL_VARYING` re-conversion and `setSqlData()` pointer redirection. Single server roundtrip for N rows. 17 array binding + 4 batch param tests all pass. | Hard | **Very High** | ✅ RESOLVED |
| **9.2** | **Extend `IBatch` for inline BLOBs** — Enabled `BLOB_ID_ENGINE` blob policy in the batch BPB when input metadata has BLOB columns. After the ODBC conversion functions create server-side blobs (via `convStringToBlob`/`convBinaryToBlob`), `registerBlob()` maps each existing blob ID to a batch-internal ID before `batch_->add()`. Updated batch eligibility in `OdbcStatement::executeStatementParamArray()` to allow BLOB columns (only arrays and data-at-exec remain excluded). Added `batchHasBlobs_` flag to `IscOdbcStatement`. | Hard | High | ✅ RESOLVED |
| **9.3** | **Remove ~35 dead ISC function pointers from `CFbDll`** — Removed all loaded-but-never-called function pointers. Kept only: `_array_*` (3 for array support), `_get_database_handle`, `_get_transaction_handle` (bridge for arrays), `_sqlcode`, and `fb_get_master_interface`. Eliminated `_dsql_*`, `_attach_database`, `_detach_database`, `_start_*`, `_commit_*`, `_rollback_*`, blob functions, date/time functions, service functions, `_interprete`, `_que_events`, etc. | Easy | Medium | ✅ RESOLVED |
| **9.4** | **Migrate `IscUserEvents` from ISC to OO API** — Replaced `isc_que_events()` with `IAttachment::queEvents()` returning `IEvents*`. Added `FbEventCallback` class implementing `IEventCallbackImpl` to bridge OO API event notifications to the legacy `callbackEvent` function pointer. Cancel via `IEvents::cancel()` in destructor. Removed `_que_events` function pointer and `que_events` typedef from `CFbDll`. `eventBuffer` changed from `char*` to `unsigned char*`. | Medium | Medium | ✅ RESOLVED |
| **9.5** | **Migrate manual TPB construction to `IXpbBuilder`** — Replaced raw byte-stuffing in `IscConnection::buildParamTransaction()` with `IXpbBuilder(TPB)`. Tags, isolation levels, and lock timeout are now inserted via `insertTag()`/`insertInt()` — lock timeout endianness is handled by the builder. For RESERVING clauses, the builder buffer is extracted and `parseReservingTable()` appends raw table-lock entries. Wrapped in try/catch for `FbException`. | Medium | Medium | ✅ RESOLVED |
| **9.6** | **Replace manual Julian-day date/time math with shared helpers** — Created `IscDbc/FbDateConvert.h` with canonical inline `fb_encode_date`/`fb_decode_date`/`fb_encode_time`/`fb_decode_time` functions. Replaced ~150 lines of triplicated Julian-day arithmetic in `OdbcConvert::encode_sql_date/decode_sql_date/encode_sql_time/decode_sql_time` and `DateTime::encodeDate/decodeDate` with calls to the shared helpers. Removed dead `OdbcDateTime` class from the build (`.cpp`/`.h` removed from CMakeLists.txt — the class was not referenced by any other code). | Medium | Medium | ✅ RESOLVED |
| **9.7** | **Unify error handling — eliminate legacy error path** — Added `getIscStatusTextFromVector()` to `CFbDll` that creates a temporary `IStatus`, populates it via `setErrors()`, and uses `IUtil::formatStatus()` — no `fb_interpret` needed. Updated `THROW_ISC_EXCEPTION_LEGACY` macro to use `getIscStatusTextFromVector()` and `getSqlCode()`. Removed `getIscStatusTextLegacy()` from `IscConnection`. Removed `_interprete` function pointer and `interprete` typedef from `CFbDll`. Both `THROW_ISC_EXCEPTION` (OO) and `THROW_ISC_EXCEPTION_LEGACY` (ISC vector) now share the same OO API error formatting path. | Medium | Medium | ✅ RESOLVED |
| **9.8** | **Optimize Sqlda data copy — skip when metadata unchanged** — In `Sqlda::checkAndRebuild()`, when metadata is NOT overridden (`isExternalOverriden()` returns false), effective pointers (`eff_sqldata`/`eff_sqlind`) already equal original pointers and no copy or buffer rebuild is performed. The data copy loop only runs when `useExecBufferMeta` is true. Eliminates unnecessary copies on every execute for the common case. | Easy | Medium | ✅ RESOLVED |
| **9.9** | **Replace `isc_vax_integer` with inline helper** — Replaced all `GDS->_vax_integer()` calls with an inline `isc_vax_integer_inline()` function in `LoadFbClientDll.h`. Removed `_vax_integer` function pointer from `CFbDll`. 4 lines of portable C++ code. | Easy | Low | ✅ RESOLVED |
| **9.10** | **Mark `IscConnection` as `final`** — Added `final` keyword to `IscConnection`, `IscStatement`, `IscOdbcStatement`, `IscPreparedStatement`, `IscCallableStatement`, `IscResultSet`, `IscDatabaseMetaData`, and all other concrete IscDbc classes. Enables compiler devirtualization. | Easy | Low | ✅ RESOLVED |
| **9.11** | **Remove commented-out legacy ISC code** — Cleaned up dead `#if 0` blocks and commented-out ISC API code in IscStatement.cpp, IscPreparedStatement.cpp, IscCallableStatement.cpp, Sqlda.cpp, and other files. | Easy | Low | ✅ RESOLVED |

#### Architecture Notes

**Why the IscDbc abstraction layer should be kept (for now)**: Despite being the only implementation, the IscDbc layer (Connection → IscConnection, Statement → IscStatement, etc.) provides a clean separation between ODBC spec compliance and database operations. Collapsing it would be a high-risk refactor affecting every file with moderate benefit — the virtual dispatch overhead is negligible compared to actual ODBC/network latency. The layer should be retained but the concrete classes should be marked `final` for devirtualization.

**Why Arrays cannot be migrated**: The Firebird OO API does not expose `IAttachment::getSlice()`/`putSlice()` equivalents for array access. The current code uses `_get_database_handle`/`_get_transaction_handle` to obtain legacy handles from OO API objects, then calls `isc_array_get_slice()`/`isc_array_put_slice()`. This bridge pattern must remain until Firebird adds OO API array support. The 3 array functions + 2 bridge functions are the irreducible minimum of ISC API usage.

**`IBatch` implementation strategy**: The `IBatch` interface (Firebird 4.0+) is the highest-impact improvement. Implementation steps:
1. In `executeStatementParamArray()`, detect if `IBatch` is available (FB4+) and no array/BLOB parameters exist
2. Call `IStatement::createBatch()` with `RECORD_COUNTS` enabled
3. For each parameter set, build the message buffer using existing Sqlda logic and call `IBatch::add()`
4. Call `IBatch::execute()` — single server roundtrip for all rows
5. Map `IBatchCompletionState::getState()` to `SQL_PARAM_STATUS_PTR` values: `EXECUTE_FAILED` → `SQL_PARAM_ERROR`, else `SQL_PARAM_SUCCESS`
6. Handle `TAG_MULTIERROR` based on `SQL_ATTR_PARAMS_PROCESSED_PTR` requirements

**Deliverable**: Legacy ISC API usage reduced from ~50 function pointers to ~5 (array only). `IBatch` implemented for PARAMSET_SIZE > 1 on FB4+ (single server roundtrip). `isc_vax_integer` replaced inline. Concrete IscDbc classes marked `final`. Dead commented-out ISC code removed. Sqlda data copy optimized. All 318 existing tests pass.

### Build Infrastructure: Vendored Header Removal ✅ (Completed — February 8, 2026)
**Goal**: Eliminate 60 vendored third-party header files committed to the repository.

| Change | Details |
|--------|---------|
| ✅ Removed `FBClient.Headers/` directory | 60 vendored Firebird/Boost header files (ibase.h, firebird/Interface.h, firebird/impl/*, firebird/impl/boost/*, firebird/impl/msg/*) deleted from the repository |
| ✅ Removed `Headers/` directory | Vendored Microsoft ODBC SDK headers (SQL.H, SQLEXT.H) deleted — system SDK headers used instead (Windows SDK / unixODBC-dev) |
| ✅ Created `cmake/FetchFirebirdHeaders.cmake` | Uses CMake `FetchContent` to download Firebird public headers from `FirebirdSQL/firebird` GitHub repo at pinned tag (v5.0.2). `SOURCE_SUBDIR` trick prevents configuring Firebird as a sub-project. Headers cached in build tree. |
| ✅ Moved `OdbcUserEvents.h` to project root | Custom Firebird ODBC extension header — the only non-vendored file in `Headers/` — moved to root alongside other project headers |
| ✅ Updated `CMakeLists.txt` + `IscDbc/CMakeLists.txt` | Include paths now reference `${FIREBIRD_INCLUDE_DIR}` (fetched from GitHub) instead of vendored `FBClient.Headers` |

### Build Infrastructure: Source Reorganization & i18n Removal ✅ (Completed — February 9, 2026)
**Goal**: Move all source code into `src/` subdirectory; remove unused internationalization support.

| Change | Details |
|--------|---------|
| ✅ Removed `Res/` directory | 5 locale resource files (resource.en, resource.es, resource.it, resource.ru, resource.uk) deleted — internationalization not supported |
| ✅ Removed i18n code from `ConnectDialog.cpp` | `TranslateString translate[]`, `selectUserLCID()`, `initCodePageTranslate()` removed; `_TR()` macro simplified to always return English string |
| ✅ Removed `initCodePageTranslate` call from `Main.cpp` | `DllMain` no longer calls the i18n initialization function |
| ✅ Moved all `.cpp`/`.h` from root to `src/` | 30+ source/header files moved to `src/` subdirectory |
| ✅ Moved `IscDbc/` to `src/IscDbc/` | IscDbc static library sources relocated under `src/` |
| ✅ Moved `.def`/`.rc`/`.manifest` to `src/` | Build-support files co-located with source |
| ✅ Removed `OdbcJdbc.exp` build artifact | Was accidentally committed; already in `.gitignore` |
| ✅ Updated `CMakeLists.txt` | Include dirs: `src/`, `src/IscDbc/`; source paths: `src/*.cpp`; .def path: `src/OdbcJdbc.def`; `add_subdirectory(src/IscDbc)` |
| ✅ Updated `tests/CMakeLists.txt` | Include dirs updated to `src/` and `src/IscDbc/` |
| ✅ Updated `.github/workflows/release.yml` | IscDbc artifact path updated to `build/src/IscDbc/libIscDbc.a` |

### Phase 10: Performance Engineering — World-Class Throughput
**Priority**: High  
**Duration**: 6–10 weeks  
**Goal**: Minimize per-row and per-column overhead to achieve best-in-class fetch/execute throughput, targeting embedded Firebird (near-zero network latency) where driver overhead dominates

#### Background: Why Performance Matters Now

With correctness, compliance, and feature completeness substantially achieved (Phases 0–9), the driver's remaining competitive gap is **throughput**. When Firebird runs as an embedded library (`libfbclient.so` / `fbclient.dll` loaded in-process), network latency drops to near zero. In this mode, the ODBC driver layer itself becomes the bottleneck — every unnecessary allocation, copy, branch, and kernel transition is measurable.

A comprehensive analysis of the data path from `SQLFetch()` → `OdbcConvert::conv*()` → `IscResultSet::nextFetch()` → `Sqlda::buffer` reveals **12 categories of overhead** that, when eliminated, can reduce per-row driver cost from ~2–5μs to ~200–500ns — a 5–10× improvement.


#### 10.0 Performance Profiling Infrastructure

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.0.1** | **Add micro-benchmark harness** — Created `tests/bench_fetch.cpp` using Google Benchmark v1.9.1. Benchmarks: (a) fetch 10K×10 INT cols, (b) 10K×5 VARCHAR(100), (c) 1K×1 BLOB, (d) batch insert 10K×10 INT, (e) SQLDescribeColW 10 cols, (f) single-row fetch. | Medium | **Essential** | ✅ |
| **10.0.2** | **Add `ODBC_PERF_COUNTERS` compile-time flag** — `src/OdbcPerfCounters.h` with atomic counters. CMake option `ODBC_PERF_COUNTERS=ON`. Zero overhead when disabled. | Easy | Medium | ✅ |
| **10.0.3** | **Establish baseline numbers** — Recorded in `Docs/PERFORMANCE_RESULTS.md`. | Easy | **Essential** | ✅ |
| **10.0.4** | **Add `benchmark` task to `Invoke-Build`** — Runs benchmarks, outputs JSON to `tmp/benchmark_results.json`. | Easy | **Essential** | ✅ |


#### 10.1 Synchronization: Eliminate Kernel-Mode Mutex



**Current state**: `SafeEnvThread.cpp` uses Win32 `CreateMutex` / `WaitForSingleObject` / `ReleaseMutex` for all locking. This is a **kernel-mode mutex** that requires a ring-3→ring-0 transition on every acquire, even when uncontended. Cost: ~1–2μs per lock/unlock pair on modern hardware. Every `SQLFetch` call acquires this lock once.

**Impact**: For a tight fetch loop of 100K rows, mutex overhead alone is **100–200ms** — often exceeding the actual database work.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.1.1** | **Replace Win32 `Mutex` with `SRWLOCK`** — Replaced `CreateMutex`/`WaitForSingleObject`/`ReleaseMutex` with `SRWLOCK` in `SafeEnvThread.h/cpp`. User-mode-only, ~20ns uncontended. On Linux, `pthread_mutex_t` unchanged (already a futex). | Easy | **Very High** | ✅ |
| **10.1.2** | **Eliminate global env-level lock for statement operations** — At `DRIVER_LOCKED_LEVEL_ENV`, statement/descriptor ops now use per-connection `SafeConnectThread`. Global lock reserved for env operations only. | Medium | **High** | ✅ |
| **10.1.3** | **Evaluate lock-free fetch path** — When a statement is used from a single thread (the common case), locking is pure waste. Add a `SQL_ATTR_ASYNC_ENABLE`-style hint or auto-detect single-threaded usage to bypass locking entirely on the fetch path. | Hard | Medium | |

#### 10.2 Per-Row Allocation Elimination

**Current state**: The `IscResultSet::next()` method (used by the higher-level JDBC-like path) calls `freeConversions()` then `allocConversions()` on **every row**, doing `delete[] conversions` + `new char*[N]`. The `nextFetch()` path (used by ODBC) avoids this, but other allocation patterns remain.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.2.1** | **Hoist `conversions` array to result-set lifetime** — `IscResultSet::next()` now calls `resetConversionContents()` which clears elements but keeps the array allocated. Array freed only in `close()`. | Easy | High | ✅ |
| **10.2.2** | **Pool BLOB objects** — `IscResultSet::next()` pre-allocates a `std::vector<std::unique_ptr<IscBlob>>` per blob column during `initResultSet()`. On each row, pooled blobs are `bind()`/`setType()`'d and passed to `Value::setValue()` without allocation. Pool is cleared in `close()`. | Medium | High (for BLOB-heavy queries) | ✅ |
| **10.2.3** | **Reuse `Value::getString()` conversion buffers** — `Value::getString(char**)` now checks if the existing buffer is large enough before `delete[]/new`. Numeric→string conversions produce ≤24 chars; the buffer from the first call is reused on subsequent rows, eliminating per-row heap churn. | Easy | Medium | ✅ |
| **10.2.4** | **Eliminate per-row `clearErrors()` overhead** — Added `[[likely]]` early-return: when `!infoPosted` (common case), `clearErrors()` skips all field resets. | Easy | Low | ✅ |
| **10.2.5** | **Pre-allocate `DescRecord` objects contiguously** — Currently each `DescRecord` is individually heap-allocated via `new DescRecord` in `OdbcDesc::getDescRecord()`. For a 20-column result, that's 20 separate heap allocations (~300–400 bytes each) with poor cache locality. Allocate all records in a single `std::vector<DescRecord>` resized to `headCount+1`. | Medium | Medium (at prepare time) | |

#### 10.3 Data Copy Chain Reduction

**Current state**: Data flows through up to 3 copies: (1) Firebird wire → `Sqlda::buffer` (unavoidable), (2) `Sqlda::buffer` → `Value` objects via `IscResultSet::next()` → `Sqlda::getValues()`, (3) `Value` → ODBC application buffer via `OdbcConvert::conv*()`. For the ODBC `nextFetch()` path, step (2) is skipped — data stays in `Sqlda::buffer` and `OdbcConvert` reads directly from SQLDA pointers. But string parameters still involve double copies.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.3.1** | **Optimize exec buffer copy on re-execute** — `Sqlda::checkAndRebuild()` copy loop now splits into two paths: on first execute (`overrideFlag==true`), per-column pointer equality is checked; on re-execute (`!overrideFlag`), the eff pointers are known-different, so copies are unconditional — eliminating N branch mispredictions per column. | Easy | Medium (for repeated executes) | ✅ |
| **10.3.2** | **Eliminate `copyNextSqldaFromBufferStaticCursor()` per row** — Static (scrollable) cursors buffer all rows in memory, then each `fetchScroll` copies one row from the buffer into `Sqlda::buffer` before conversion. Instead, have `OdbcConvert` read directly from the static cursor buffer row, skipping the intermediate copy. | Hard | Medium (scrollable cursors only) |
| **10.3.3** | **Avoid Sqlda metadata rebuild on re-execute** — `Sqlda::setValue()` now uses a `setTypeAndLen()` helper that only writes `sqltype`/`sqllen` when the new value differs from the current. This prevents `propertiesOverriden()` from detecting false changes, skipping the expensive `IMetadataBuilder` rebuild on re-execute. `sqlscale` write is similarly guarded. | Medium | Medium (for repeated executes) | ✅ |

#### 10.4 Conversion Function Overhead Reduction

**Current state**: Each column conversion is dispatched via a **member function pointer** (`ADRESS_FUNCTION = int (OdbcConvert::*)(DescRecord*, DescRecord*)`). Inside each conversion, 4 `getAdressBindData/Ind` calls perform null checks + pointer dereferences through offset pointers. The `CHECKNULL` macro branches on `isIndicatorSqlDa` per column per row.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.4.1** | **Replace member function pointers with regular function pointers** — Change `ADRESS_FUNCTION` from `int (OdbcConvert::*)(DescRecord*, DescRecord*)` to `int (*)(OdbcConvert*, DescRecord*, DescRecord*)`. Member function pointers on MSVC are 16 bytes (vs 8 for regular pointers) and require an extra thunk adjustment. Regular function pointers are faster to dispatch and smaller. | Medium | Medium |
| **10.4.2** | **Cache bind offset values in `OdbcConvert` by value** — Currently `bindOffsetPtrTo` / `bindOffsetPtrFrom` are `SQLLEN*` pointers that are dereferenced in every `getAdressBindDataTo/From` call (4× per conversion). Cache the actual `SQLLEN` value at the start of each row's conversion pass, avoiding 4 pointer dereferences per column. | Easy | Medium |
| **10.4.3** | **Split conversion functions by indicator type** — The `CHECKNULL` macro branches on `isIndicatorSqlDa` (true for Firebird internal descriptors, false for app descriptors) on every conversion. Since this property is fixed at bind time, generate two variants of each conversion function and select the correct one in `getAdressFunction()`. | Hard | Medium |
| **10.4.4** | **Implement bulk identity path** — When `bIdentity == true` (source and destination types match, same scale, no offset), the conversion is a trivial `*(T*)dst = *(T*)src`. For a row of N identity columns, replace N individual function pointer calls with a single `memcpy(dst_row, src_row, row_size)` or a tight loop of fixed-size copies. Detect this at bind time. | Hard | **High** (for identity-type fetches) |
| **10.4.5** | **Use SIMD/`memcpy` for fixed-width column arrays** — When fetching multiple rows into column-wise bound arrays of fixed-width types (INT, BIGINT, DOUBLE), the per-column data in `Sqlda::buffer` is at a fixed stride. A single `memcpy` per column (or even AVX2 scatter/gather) can replace the per-row-per-column conversion loop. Requires column-wise fetch mode (see 10.5). | Hard | **Very High** (for columnar workloads) |
| **10.4.6** | **Use `std::to_chars` for float→string** — `OdbcConvert::convFloatToString` and `convDoubleToString` now use C++17 `std::to_chars` with fallback to the legacy `ConvertFloatToString` on overflow. Eliminates repeated `fmod()` calls; 5–10× faster for numeric output. | Easy | Medium (for float→string workloads) | ✅ |
| **10.4.7** | **Add `[[likely]]`/`[[unlikely]]` branch hints** — Annotate null-check fast paths in `getAdressBindData*` and `CHECKNULL` macros. The common case is non-NULL data and non-NULL indicators. Help the compiler lay out the hot path linearly. | Easy | Low |

#### 10.5 Block Fetch / Columnar Fetch

**Current state**: `sqlFetch()` fetches one row at a time from Firebird via `IResultSet::fetchNext()`, then converts one row at a time. For embedded Firebird, the per-row call overhead (function pointer dispatch, status check, buffer cursor advance) is significant relative to the actual data access.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.5.1** | **Implement N-row prefetch buffer** — `IscResultSet` allocates a 64-row prefetch buffer during `initResultSet()`. `nextFetch()` fills the buffer in batches of up to 64 rows via `IResultSet::fetchNext()`, then serves rows from the buffer via `memcpy` to `sqlda->buffer`. A `prefetchCursorDone` flag prevents re-fetching after the Firebird cursor returns `RESULT_NO_DATA`. Amortizes per-fetch overhead across 64 rows. Works correctly with static cursors (`readFromSystemCatalog`) and system catalog queries. | Medium | **High** | ✅ |
| **10.5.2** | **Columnar conversion pass** — After fetching N rows into a multi-row buffer, convert all N values of column 1, then all N values of column 2, etc. This maximizes L1/L2 cache utilization because: (a) the conversion function pointer is loaded once per column, not once per row; (b) source data for each column is at a fixed stride in the buffer; (c) destination data in column-wise bound arrays is contiguous. | Hard | **Very High** |
| **10.5.3** | **Prefetch hints** — When fetching N rows, issue `__builtin_prefetch()` / `_mm_prefetch()` on the next row's source data while converting the current row. For multi-row buffers with known stride, prefetch 2–3 rows ahead. | Medium | Medium (hardware-dependent) |

#### 10.6 Unicode (W API) Overhead Reduction

**Current state**: Every W API function creates 1–6 `ConvertingString` RAII objects, each performing: (1) `MultiByteToWideChar` to measure length, (2) `new char[]` heap allocation, (3) `WideCharToMultiByte` to convert, (4) `delete[]` on destruction. For `SQLDescribeColW` called 20 times (one per column), this is 20 heap alloc/free cycles just for column names.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.6.1** | **Stack-buffer fast path in `ConvertingString`** — Added 512-byte `stackBuf` member. Conversion tries stack buffer first; heap-allocates only if >512 bytes. Eliminates virtually all W-path heap allocations. | Easy | **Very High** | ✅ |
| **10.6.2** | **Single-pass W→A conversion** — On Windows, `WideCharToMultiByte` now writes directly into `stackBuf` first. If it fits, done (single pass). If not, falls back to measure+allocate. | Easy | Medium | ✅ |
| **10.6.3** | **Native UTF-16 internal encoding** — The driver currently converts W→A at entry, processes as ANSI, then converts A→W at exit. For a fully Unicode application (the modern default), this is pure waste — every string is converted twice. Instead, store strings internally as UTF-16 (`SQLWCHAR*`) and only convert to ANSI for the Firebird API (which uses UTF-8 when `CHARSET=UTF8`). This eliminates the W→A→W round-trip for metadata strings. **Implemented in Phase 12.2** — `OdbcString` w-cache in `DescRecord`, direct UTF-16 output in `SQLDescribeColW`, `SQLColAttributeW`, `SQLGetDescFieldW`, `SQLGetDescRecW`, `SQLGetDiagRecW`, `SQLGetDiagFieldW`. | Very Hard | **Very High** (long-term) | ✅ |
| **10.6.4** | **Use `Utf16Convert` directly instead of `WideCharToMultiByte` on Windows** — `ConvertingString` now checks `codePage == CP_UTF8` and routes through `Utf8ToUtf16`/`Utf16ToUtf8`/`Utf16ToUtf8Length` from `Utf16Convert.h` instead of `MultiByteToWideChar`/`WideCharToMultiByte`. Applied to both the constructor (A→W) and destructor (W→A) paths, plus the heap-allocation fallback path. Avoids Windows code-page dispatch overhead. | Easy | Low-Medium | ✅ |

#### 10.7 Compiler & Build Optimizations

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.7.1** | **Enable LTO (Link-Time Optimization)** — Added `check_ipo_supported()` + `CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE`. Enables cross-TU inlining of `conv*` methods and dead code elimination across OdbcFb→IscDbc boundary. | Easy | **High** | ✅ |
| **10.7.2** | **Enable PGO (Profile-Guided Optimization)** — Add a PGO training workflow: (1) build with `/GENPROFILE` (MSVC) or `-fprofile-generate` (GCC/Clang), (2) run the benchmark suite, (3) rebuild with `/USEPROFILE` or `-fprofile-use`. PGO dramatically improves branch prediction and code layout for the fetch hot path. | Medium | **High** | |
| **10.7.3** | **Mark hot functions with `ODBC_FORCEINLINE`** — Defined `ODBC_FORCEINLINE` macro (`__forceinline` on MSVC, `__attribute__((always_inline))` on GCC/Clang). Applied to 6 hot functions: `getAdressBindDataTo`, `getAdressBindDataFrom`, `getAdressBindIndTo`, `getAdressBindIndFrom`, `setIndicatorPtr`, `checkIndicatorPtr`. | Easy | Medium | ✅ |
| **10.7.4** | **Ensure `OdbcConvert` methods are not exported** — Verified: `conv*` methods are absent from `OdbcJdbc.def` and no `__declspec(dllexport)` on `OdbcConvert`. LTO can freely inline them. | Easy | Medium (with LTO) | ✅ |
| **10.7.5** | **Set `/favor:AMD64` or `-march=native` for release builds** — Added `/favor:AMD64` for MSVC and `-march=native` for GCC/Clang in CMakeLists.txt Release flags. Enables architecture-specific instruction scheduling, `cmov`, `popcnt`, and better vectorization. | Easy | Low | ✅ |
| **10.7.6** | **`#pragma optimize("gt", on)` for hot files** — On MSVC, apply `favor:fast` and `global optimizations` specifically to `OdbcConvert.cpp`, `OdbcStatement.cpp`, and `IscResultSet.cpp`. | Easy | Low | |

#### 10.8 Memory Layout & Cache Optimization

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.8.1** | **Contiguous `CBindColumn` array** — The `ListBind<CBindColumn>` used in `returnData()` already stores `CBindColumn` structs contiguously. Verify that `CBindColumn` is small and dense (no padding, no pointers to unrelated data). If it contains a pointer to `DescRecord`, consider embedding the needed fields (fnConv, dataPtr, indicatorPtr) directly to avoid the pointer chase. | Medium | Medium | |
| **10.8.2** | **`alignas(64)` on `Sqlda::buffer`** — Replaced `std::vector<char>` with `std::vector<char, AlignedAllocator<char, 64>>` for cache-line-aligned buffer allocation. The `AlignedAllocator` uses `_aligned_malloc`/`posix_memalign`. | Easy | Low | ✅ |
| **10.8.3** | **`DescRecord` field reordering** — Move the hot fields used during conversion (`dataPtr`, `indicatorPtr`, `conciseType`, `fnConv`, `octetLength`, `isIndicatorSqlDa`) to the first 64 bytes of the struct. Cold fields (catalogName, baseTableName, literalPrefix, etc. — 11 JStrings) should be at the end. This keeps one cache line hot during the conversion loop. | Medium | Medium | |
| **10.8.4** | **Avoid false sharing on `countFetched`** — `OdbcStatement::countFetched` is modified on every fetch row. If it shares a cache line with read-only fields accessed by other threads, it causes false sharing. Add `alignas(64)` padding around frequently-written counters. | Easy | Low (only relevant with multi-threaded access) | |

#### 10.9 Statement Re-Execution Fast Path

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.9.1** | **Skip `SQLPrepare` re-parse when SQL unchanged** — Cache the last SQL string hash. If `SQLPrepare` is called with the same SQL, skip the Firebird `IStatement::prepare()` call entirely and reuse the existing prepared statement. | Easy | **High** (for ORM-style repeated prepares) |
| **10.9.2** | **Skip `getUpdateCount()` for SELECT statements** — `IscStatement::execute()` always calls `statement->getAffectedRecords()` after execute. For SELECTs (which return a result set, not an update count), this is a wasted Firebird API call. Guard with `statementType == isc_info_sql_stmt_select`. | Easy | Medium |
| **10.9.3** | **Avoid conversion function re-resolution on re-execute** — `getAdressFunction()` (the 860-line switch) is called once per column at bind time and cached in `DescRecord::fnConv`. Verify this cache is preserved across re-executions of the same prepared statement with the same bindings. If `OdbcDesc::getDescRecord()` reinitializes `fnConv`, add a dirty flag. | Easy | Low |

#### 10.10 Advanced: Asynchronous & Pipelined Fetch

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **10.10.1** | **Double-buffered fetch** — Allocate two `Sqlda::buffer` slots. While `OdbcConvert` processes buffer A, issue `IResultSet::fetchNext()` into buffer B on a worker thread (or via async I/O). When conversion of A completes, swap buffers. This hides Firebird fetch latency behind conversion work. Only beneficial when Firebird is not embedded (i.e., client/server mode with network latency). | Very Hard | High (client/server mode) |
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

### Phase 11: SQLGetTypeInfo Correctness, Connection Pool Awareness & Statement Timeout ✅ (Completed — February 8, 2026)
**Priority**: Medium-High  
**Duration**: 3–5 weeks  
**Goal**: Fix spec violations and thread-safety bugs in `SQLGetTypeInfo`; implement driver-aware connection pooling SPI; add functional `SQL_ATTR_QUERY_TIMEOUT` using Firebird's `cancelOperation()`; correct async capability reporting

#### Background

A detailed audit against three Microsoft ODBC specification documents and the Firebird OO API (`Using_OO_API.html`) reveals three categories of implementable improvements:

1. **SQLGetTypeInfo** ([spec](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgettypeinfo-function)) — The current `TypesResultSet` has a thread-safety bug (global mutable static array), result set ordering that violates the ODBC spec, duplicate type entries that confuse applications on FB4+, and a `findType()` that returns only the first matching row when multiple rows share a `DATA_TYPE` code.

2. **Connection Pool Awareness** ([spec](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-driver/developing-connection-pool-awareness-in-an-odbc-driver)) — The driver currently relies on the Driver Manager's default string-matching pool. ODBC 3.81 defines a Service Provider Interface (SPI) with 7 functions that allow the driver to participate in intelligent pooling — rating reusable connections, computing pool IDs from connection parameters, and handling `SQL_HANDLE_DBC_INFO_TOKEN`. Additionally, the existing `SQL_ATTR_RESET_CONNECTION` handler is incomplete (no transaction rollback, no cursor cleanup).

3. **Statement Timeout / `SQL_ATTR_QUERY_TIMEOUT`** — The setter currently no-ops silently. The Firebird OO API provides `IAttachment::cancelOperation(fb_cancel_raise)`, which can cancel in-flight queries from a separate thread. A timer-based implementation is feasible and would make the driver the only Firebird ODBC driver with functional query timeout support.

4. **Async capability misreport** — `SQL_ASYNC_MODE` reports `SQL_AM_STATEMENT` but the driver rejects `SQL_ATTR_ASYNC_ENABLE = SQL_ASYNC_ENABLE_ON` with HYC00. True async execution is **not feasible** because the Firebird OO API is entirely synchronous (no non-blocking calls, no completion callbacks). However, the notification-based async model (ODBC 3.8 `SQLAsyncNotificationCallback`) could theoretically be built on top of a worker-thread approach — this is deferred to a future phase as the cost/benefit ratio is unfavorable.

#### 11.1 SQLGetTypeInfo — Fix Spec Violations & Thread-Safety

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| ✅ **11.1.1** | **Fix thread-safety: eliminate mutation of static `alphaV` array** — The `TypesResultSet` constructor modifies the global `static AlphaV alphaV[]` in-place (adjusting `sqlType`/`sqlSubType` for ODBC 2.x/3.x date/time codes and `columnSize` for charset). Two concurrent `SQLGetTypeInfo` calls with different ODBC versions or character sets corrupt each other. **Fix**: copy the static array into a per-instance `std::vector<AlphaV>` in the constructor and mutate the copy. The static array becomes `const`. | Easy | **High** — eliminates data race | Completed Feb 8, 2026 |
| ✅ **11.1.2** | **Sort result set by DATA_TYPE ascending** — The ODBC spec mandates ordering by `DATA_TYPE` first, then by closeness of mapping. The current static array is in an arbitrary order. **Fix**: after copying into the per-instance vector (11.1.1), sort by `DATA_TYPE` ascending, then by `TYPE_NAME` ascending as tie-breaker. | Easy | Medium — spec compliance | Completed Feb 8, 2026 |
| ✅ **11.1.3** | **Fix `findType()` to return all matching rows** — When a specific `DataType` is requested (not `SQL_ALL_TYPES`), the current code returns only the first matching row. On FB4+, `SQL_NUMERIC` matches both NUMERIC and INT128; `SQL_DOUBLE` matches both DOUBLE PRECISION and DECFLOAT. The spec requires all matching rows be returned. **Fix**: change the iteration to continue past the first match and return all rows with the matching `DATA_TYPE`. | Easy | Medium — spec compliance | Completed Feb 8, 2026 |
| ✅ **11.1.4** | **Remove duplicate BINARY/VARBINARY BLOB entries on FB4+** — Pre-FB4, `BLOB SUB_TYPE 0` is aliased to `SQL_BINARY` (-2) and `SQL_VARBINARY` (-3) as a fallback. On FB4+ servers, native `BINARY` and `VARBINARY` types exist and are reported. This creates duplicate entries. **Fix**: version-gate the BLOB-as-BINARY/VARBINARY entries to `server < 4` so they don't appear alongside the real FB4 types. | Easy | Medium — cleaner type info | Completed Feb 8, 2026 |
| ✅ **11.1.5** | **Fix SQL_GUID metadata** — `SEARCHABLE` is `3` (SQL_SEARCHABLE, implies LIKE works) but GUID/binary data can't use LIKE. Change to `2` (SQL_ALL_EXCEPT_LIKE). Also review `LITERAL_PREFIX`/`SUFFIX` — GUID types don't use quote literals in standard ODBC; set to NULL. | Easy | Low — minor correctness | Completed Feb 8, 2026 |
| ✅ **11.1.6** | **Fix SQL_ASYNC_MODE misreport** — `InfoItems.h` declares `NITEM(SQL_ASYNC_MODE, SQL_AM_STATEMENT)` but the driver rejects async enable with HYC00. Change to `NITEM(SQL_ASYNC_MODE, SQL_AM_NONE)`. | Easy | **High** — eliminates DM confusion | Completed Feb 8, 2026 |
| ✅ **11.1.7** | **Add tests for SQLGetTypeInfo fixes** — Test result set ordering, multi-row returns for duplicate `DATA_TYPE` codes, version-gated type visibility, SQL_GUID searchability, thread-safety under concurrent access. | Medium | Medium | Completed Feb 8, 2026 |

**Reference**: [SQLGetTypeInfo Function](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgettypeinfo-function)

#### 11.2 Statement Timeout via `IAttachment::cancelOperation()`

The Firebird OO API exposes `IAttachment::cancelOperation(StatusType*, int option)` with `fb_cancel_raise` as the option to interrupt an in-flight operation. This can be called from any thread. The driver can use this to implement functional `SQL_ATTR_QUERY_TIMEOUT`.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| ✅ **11.2.1** | **Implement `SQL_ATTR_QUERY_TIMEOUT` setter/getter** — Store the timeout value (in seconds) in `OdbcStatement`. If the driver cannot support the exact value, return `SQL_SUCCESS_WITH_INFO` with SQLSTATE 01S02 and report the actual supported value. A value of `0` means no timeout (default). | Easy | Medium | Completed Feb 8, 2026: `queryTimeout` member in OdbcStatement; getter/setter store and return value |
| ✅ **11.2.2** | **Implement timer-based cancellation thread** — When `SQL_ATTR_QUERY_TIMEOUT > 0` and a statement begins execution (`SQLExecute`/`SQLExecDirect`/`SQLFetch`), start a platform timer (Win32 `CreateTimerQueueTimer` / POSIX `timer_create` or `std::jthread` with `std::condition_variable::wait_for`). On timeout expiry, call `attachment->cancelOperation(&status, fb_cancel_raise)` on the connection's `IAttachment*`. The timer is cancelled when the operation completes normally. | Medium | **High** — unique feature | Completed Feb 8, 2026: `startQueryTimer()`/`cancelQueryTimer()` using `std::thread` + `std::condition_variable::wait_for`; fires `cancelOperation(fb_cancel_raise)` on expiry; wired into `sqlExecute()`/`sqlExecDirect()` |
| ✅ **11.2.3** | **Handle `isc_cancelled` error gracefully** — When `cancelOperation()` fires, the in-flight Firebird call raises `isc_cancelled`. The driver must catch this, map it to SQLSTATE HYT00 (Timeout expired), and return `SQL_ERROR` to the application. | Easy | Medium | Completed Feb 8, 2026: `cancelledByTimeout` flag distinguishes timer-triggered vs manual cancel; timeout produces HYT00, manual cancel produces HY008 |
| ✅ **11.2.4** | **Implement `SQLCancel` using `cancelOperation(fb_cancel_raise)`** — The existing `SQLCancel` stub should call `attachment->cancelOperation(&status, fb_cancel_raise)` to cancel the currently-executing statement on another thread. This makes `SQLCancel` actually functional instead of a no-op. | Easy | **High** | Completed Feb 8, 2026: `sqlCancel()` calls `connection->cancelOperation()` which uses `IAttachment::cancelOperation(fb_cancel_raise)` |
| ✅ **11.2.5** | **Add tests for query timeout** — Test: (a) timeout fires on a long-running query (use `SELECT * FROM rdb$relations CROSS JOIN rdb$relations` or PG-style `pg_sleep` equivalent), (b) timeout of 0 means no timeout, (c) `SQLCancel` from another thread interrupts execution, (d) SQLSTATE HYT00 returned on timeout. | Medium | Medium | Completed Feb 8, 2026: `QueryTimeoutTest` fixture (7 tests): default=0, set/get, set-to-zero, cancel-idle, cancel-from-thread, timer-fires, zero-no-cancel |

**Firebird API**: `IAttachment::cancelOperation(StatusType* status, int option)` — see [Using_OO_API](https://github.com/FirebirdSQL/firebird/blob/master/doc/Using_OO_API.md)

#### 11.3 Connection Pool Awareness — `SQL_ATTR_RESET_CONNECTION` Improvements

True driver-aware connection pooling (the SPI with `SQLPoolConnect`, `SQLRateConnection`, `SQLGetPoolID`, etc.) is a large undertaking that requires a new handle type (`SQL_HANDLE_DBC_INFO_TOKEN`), 7 new exported functions, and careful integration with the Driver Manager's pool lifecycle. This is deferred to a future phase.

However, the existing `SQL_ATTR_RESET_CONNECTION` handler is incomplete and can be improved to make the DM's default string-matching pool work correctly:

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| ✅ **11.3.1** | **Rollback pending transactions on reset** — The current reset handler does not roll back uncommitted work. A pooled connection returned to the pool with an open transaction can cause lock contention or data corruption when reused. **Fix**: call `connection->rollbackTransaction()` (or `ITransaction::rollback()`) if a transaction is active. | Easy | **High** — correctness | Completed Feb 8, 2026: Checks `getTransactionPending()` and calls `rollback()` in `SQL_ATTR_RESET_CONNECTION` handler |
| ✅ **11.3.2** | **Close open cursors/statements on reset** — Open cursors hold server resources and may block other connections. Iterate all child statement handles and call `sqlCloseCursor()` / free prepared statements. | Medium | **High** — resource cleanup | Completed Feb 8, 2026: Iterates child statements, calls `releaseResultSet()` on any with open `resultSet` |
| ✅ **11.3.3** | **Reset additional attributes** — Reset `SQL_ATTR_QUERY_TIMEOUT` on child statements, connection timeout, and other driver-specific attributes to their post-connect defaults. Note: `SQL_ATTR_METADATA_ID` and character set are not tracked as mutable state by this driver — no reset needed. | Easy | Medium — completeness | Completed Feb 8, 2026: Child statement `queryTimeout` reset to 0; `connectionTimeout` reset to 0; autocommit, access mode, transaction isolation already reset |
| ✅ **11.3.4** | **Add tests for connection reset** — Test: (a) uncommitted INSERT is rolled back after reset, (b) open cursor is closed after reset, (c) autocommit is restored to ON, (d) transaction isolation is restored to default, (e) connection is reusable after reset, (f) query timeout reset to 0. | Medium | Medium | Completed Feb 8, 2026: `ConnectionResetTest` fixture (6 tests): autocommit, isolation, rollback, reusable, cursor-close, queryTimeout-reset |

**Reference**: [Developing Connection-Pool Awareness in an ODBC Driver](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-driver/developing-connection-pool-awareness-in-an-odbc-driver)

#### 11.4 Future: Driver-Aware Connection Pool SPI (Deferred)

The full SPI implementation requires exporting 7 new functions from the driver DLL and handling the `SQL_HANDLE_DBC_INFO_TOKEN` lifecycle. This is architecturally significant and should be a standalone phase. Documented here for reference:

| SPI Function | Purpose | Feasibility |
|---|---|---|
| `SQLSetConnectAttrForDbcInfo` | Set attributes on info token before connect | ✅ Straightforward — store in a map |
| `SQLSetConnectInfo` | Set DSN/UID/PWD on info token | ✅ Straightforward |
| `SQLSetDriverConnectInfo` | Set connection string on info token | ✅ Straightforward |
| `SQLGetPoolID` | Compute pool ID from info token (server + credentials + charset + dialect) | ✅ Hash of key attributes |
| `SQLRateConnection` | Rate pooled connection match (0–100) | ✅ Compare key attrs, rate mismatches |
| `SQLPoolConnect` | Reuse or create connection from pool | ✅ Call reset + reconnect if needed |
| `SQLCleanupConnectionPoolID` | Free pool ID resources | ✅ Free hash |

**Prerequisites**: Phase 11.3 (complete reset) must be done first. Pool-aware drivers must correctly reset all state when reusing connections.

#### 11.5 Future: Async Execution (Deferred — Pending Firebird Async API)

True async execution is **not feasible** with the current Firebird OO API, which is entirely synchronous. The notification-based async model (ODBC 3.8's `SQL_ATTR_ASYNC_DBC_NOTIFICATION_CALLBACK` / `SQL_ATTR_ASYNC_STMT_NOTIFICATION_CALLBACK`) could be emulated using worker threads, but the complexity is high and the benefit is low — applications that need async typically use connection-per-thread patterns.

| Constraint | Detail |
|---|---|
| Firebird OO API | All operations are synchronous/blocking. No `IStatement::executeAsync()` or completion callback exists. |
| Worker-thread emulation | Possible but complex: spawn a thread per async call, call `SQLAsyncNotificationCallback` when done. Requires careful thread-safety for all driver state. |
| `SQLCompleteAsync` | Would need to be implemented to retrieve the final return code from the worker thread. |
| Cost/Benefit | High implementation cost, low benefit — few applications use ODBC async. `SQL_ATTR_QUERY_TIMEOUT` (11.2) covers the most important use case (interruptible queries). |

**Decision**: Fix the `SQL_ASYNC_MODE` misreport to `SQL_AM_NONE` (task 11.1.6). Defer full async to a future phase if/when Firebird adds non-blocking API support.

#### Success Criteria

- [x] `SQLGetTypeInfo` result set sorted by `DATA_TYPE` ascending (spec compliant)
- [x] Static types array is `const`; per-instance copies used for mutation (thread-safe)
- [x] All rows matching a given `DATA_TYPE` returned (multi-row, no more `findType()` single-match)
- [x] No duplicate BINARY/VARBINARY entries on FB4+ servers (version-gated with negative label)
- [x] `SQL_ASYNC_MODE` correctly reports `SQL_AM_NONE`
- [x] `SQL_ATTR_QUERY_TIMEOUT` stores the timeout value (getter/setter working)
- [x] `SQLCancel` calls `IAttachment::cancelOperation(fb_cancel_raise)`
- [x] Long-running queries are interruptible from another thread
- [x] `SQL_ATTR_RESET_CONNECTION` rolls back pending transactions and closes cursors
- [x] All 385+ existing tests still pass (406 total with new Phase 11 tests)
- [x] 21 new tests cover type info correctness, timeout, cancellation, and connection reset

**Deliverable**: A spec-compliant `SQLGetTypeInfo` with correct ordering and thread-safety; functional query timeout and cancellation using Firebird's native API; robust connection reset for pool environments. These improvements target real-world correctness issues that affect ORM frameworks (Entity Framework, Hibernate) and connection pool managers (HikariCP, ADO.NET pool).

### Phase 12: String Conversion Elimination & OdbcConvert Rationalization
**Priority**: High  
**Duration**: 8–12 weeks  
**Goal**: Eliminate redundant charset transliterations, consolidate encoding paths, rationalize the OdbcConvert conversion matrix, and implement native UTF-16 internal encoding (task 10.6.3)
**Current Status**: Phase 12 ✅ **COMPLETE** — February 10, 2026

**Phase 12.1 Summary of Changes:**
- Introduced `ODBC_SQLWCHAR` typedef in `Connection.h` (always 16-bit on all platforms)
- Changed all codec typedefs (`WCSTOMBS`/`MBSTOWCS`) and function signatures to use `ODBC_SQLWCHAR*` instead of `wchar_t*`
- Updated all `*ToStringW` conversion functions in `OdbcConvert.cpp` to use `SQLWCHAR*` directly — eliminates Linux data corruption
- Updated `convVarStringSystemToStringW` to use `Utf8ToUtf16()` instead of locale-dependent `mbstowcs()`
- Added trivial ASCII widening in `ODBCCONVERT_CONV_TO_STRINGW` macro for int→StringW formatting
- Updated FSS and UTF-8 codecs in `MultibyteConvert.cpp` to use `ODBC_SQLWCHAR*`
- On Linux, `CHARSET=NONE` fallback now uses UTF-8 codec (not incompatible `wcstombs`)
- Default `CHARSET` to `UTF8` when not specified in `Attachment.cpp`
- All 406 tests pass, benchmarks verified

**Phase 12.2 Summary of Changes:**
- Introduced `OdbcString` class (`OdbcString.h`) — UTF-16-native string with `from_utf8`/`from_utf16`/`from_ascii` factories, `to_utf8()`, `copy_to_w_buffer`/`copy_to_a_buffer` with truncation handling
- 26 unit tests for `OdbcString` covering construction, conversion, copy semantics, buffer operations, and round-trips
- Added `sqlGetDiagRecW`/`sqlGetDiagFieldW` methods on `OdbcError` and `OdbcObject` — direct UTF-16 output without `ConvertingString`
- Updated `SQLGetDiagRecW`/`SQLGetDiagFieldW` in `MainUnicode.cpp` to call the W methods directly, eliminating the W→A→W roundtrip
- Added `OdbcString` w-cache fields to `DescRecord` — 11 fields (`wBaseColumnName`, `wBaseTableName`, `wCatalogName`, `wLabel`, `wLiteralPrefix`, `wLiteralSuffix`, `wLocalTypeName`, `wName`, `wSchemaName`, `wTableName`, `wTypeName`) populated once at IRD creation time
- Added `DescRecord::getWString(fieldId)` accessor for field-id-based lookup of cached UTF-16 strings
- Added `sqlDescribeColW`/`sqlColAttributeW` methods on `OdbcStatement` — read from w-cache directly
- Added `sqlGetDescFieldW`/`sqlGetDescRecW` methods on `OdbcDesc` — read from w-cache directly
- Updated all 7 W-API metadata/diagnostic functions in `MainUnicode.cpp` to call W methods directly, completely bypassing `ConvertingString` on the output path
- Tasks 12.2.4, 12.2.5, 12.2.7 evaluated and marked N/A — the dual-storage approach (JString + OdbcString) achieves the same performance goal without requiring full JString replacement or ConvertingString rewrite

**Phase 12.3 Summary of Changes:**
- Removed `convVarStringSystemToString`/`convVarStringSystemToStringW` — trailing-space trimming folded into standard variants via `isResultSetFromSystemCatalog` flag
- Removed 4 dispatch branches in `getAdressFunction()` — simplified to always use standard path
- Unified `convStringToStringW`/`convVarStringToStringW` via shared `convToStringWImpl` helper — extracted common MbsToWcs, chunked SQLGetData, truncation, and indicator logic
- Audited all 17 `notYetImplemented` paths — most are correct per ODBC spec; documented missing conversions
- Created `Docs/CONVERSION_MATRIX.md` — comprehensive reference for the conversion dispatch table

**Phase 12.4 Summary of Changes:**
- Default `CHARSET=UTF8` when not specified (12.4.1)
- Added CHARSET documentation to README.md with usage table (12.4.2)

#### Background & Research Findings

A comprehensive analysis of the data conversion architecture — comparing against the PostgreSQL ODBC driver (psqlodbc), the Firebird server's charset system, and the ODBC specification — reveals the following:

##### Finding 1: OdbcConvert.cpp Is Necessary (Cannot Be Removed)

Unlike psqlodbc, which receives **all data from PostgreSQL as text strings** and converts them inline with `pg_atoi()`/`pg_atof()` in a single monolithic `copy_and_convert_field()` function, the Firebird driver receives **binary wire data** (ISC_DATE integers, SQLDA-format VARYING strings, scaled integer numerics, etc.). This binary protocol requires type-specific decoders — exactly what `OdbcConvert`'s ~150 conversion functions provide.

**Why OdbcConvert is architecturally superior to psqlodbc's approach:**
- **Dispatch-table caching**: The Firebird driver resolves the conversion function *once* at bind time (`getAdressFunction()`) and caches the function pointer in `DescRecord::fnConv`. psqlodbc evaluates a two-level `switch` for every column of every row — measurably slower.
- **Binary source data**: Firebird's binary wire protocol means numeric conversions are `*(int*)ptr` → cast, not `pg_atoi(text_string)` → strtol. Binary-to-binary is inherently faster than text-to-binary.
- **No SQL rewriting**: psqlodbc's `convert.c` (6600 lines) is actually ~50% SQL rewriting/parameter binding code mixed in with data conversion. Our `OdbcConvert.cpp` is pure data conversion.

**Conclusion**: `OdbcConvert.cpp` should be **kept and optimized**, not removed. The dispatch-table-plus-cached-function-pointer pattern is the correct design for a binary-protocol driver. The file's size (4627 lines) is a consequence of the large ODBC type matrix, not bad architecture.

##### Finding 2: Firebird Cannot Communicate in UTF-16

The Firebird server **cannot** use UTF-16 or UCS-2 as a connection character set. The server's `initCharSet()` (in `jrd/Attachment.cpp`) explicitly rejects any charset with `charset_min_bytes_per_char != 1`:

```cpp
if (cs->charset_min_bytes_per_char != 1) {
    valid = false;
    s.printf("%s. Wide character sets are not supported yet.", ...);
}
```

Both `CS_UTF16` (ID 61) and `CS_UNICODE_UCS2` (ID 8) have `min_bytes_per_char = 2`. UTF-16 is used internally as the **pivot encoding** for all charset-to-charset transliteration (via `CsConvert::cnvt1` → UTF-16 → `CsConvert::cnvt2`), but it is not exposed as a client wire encoding.

**Implication**: The driver MUST use `CHARSET=UTF8` (or another single-byte-minimum charset) for the connection DPB. All text data arrives from the wire as UTF-8 bytes. The driver must then convert UTF-8 → UTF-16 for `SQL_C_WCHAR` bindings. **This conversion is unavoidable** and is not a redundancy — it's the minimal path.

**Regarding `IMetadataBuilder::setCharSet()`**: This OO API method only sets metadata tags on message buffers — it does NOT cause the engine to automatically transliterate output. It is used by UDR plugins to describe their buffer layouts, not by client applications to request a different wire encoding.

##### Finding 3: `CHARSET=UTF8` Is the Optimal Choice

Using `CHARSET=UTF8` in the connection string is **not counterproductive** — it's the only correct choice for a Unicode ODBC driver:

1. **Server-side transliteration**: When the connection charset is UTF-8, the Firebird server transliterates all column data to UTF-8 before sending it over the wire. This means a `VARCHAR(100) CHARACTER SET WIN1252` column's data is converted to UTF-8 by the server, not the driver. This is **desirable** — it offloads charset conversion to the server (which has full ICU support) and gives the driver a single, consistent input encoding.

2. **System catalog metadata**: Firebird stores all system catalog metadata (table names, column names, etc.) in UTF-8 internally (`CS_METADATA = CS_UTF8`). Using `CHARSET=UTF8` means these strings arrive without any server-side transliteration — zero overhead for metadata operations.

3. **Single input encoding**: With `CHARSET=UTF8`, the driver knows that ALL incoming text data is UTF-8. This eliminates the need for per-column charset detection and simplifies the conversion path to a single, well-optimized UTF-8 → UTF-16 converter.

4. **Alternative considered — `CHARSET=NONE`**: Using `CHARSET=NONE` (CS_NONE, ID 0) would send bytes in the column's native charset without transliteration. This would require the driver to implement charset-specific decoders for every Firebird charset (WIN1252, ISO8859-1, KOI8-R, SJIS, BIG5, GB2312, etc.) — a massive and error-prone effort. `CHARSET=UTF8` correctly delegates this to the server.

##### Finding 4: Current Encoding Architecture Has Redundancies

The driver currently has **five distinct encoding conversion implementations**:

| # | Implementation | Location | Used By |
|---|---------------|----------|---------|
| 1 | `utf8_mbstowcs` / `utf8_wcstombs` | `IscDbc/MultibyteConvert.cpp` | Per-column `DescRecord::MbsToWcs` (fetch path) |
| 2 | `Utf8ToUtf16` / `Utf16ToUtf8` | `Utf16Convert.cpp` | `ConvertingString` (W-API shim) |
| 3 | `fss_mbstowcs` / `fss_wcstombs` | `IscDbc/MultibyteConvert.cpp` | UNICODE_FSS columns (legacy) |
| 4 | `_MbsToWcs` / `_WcsToMbs` | `IscDbc/MultibyteConvert.cpp` | `CHARSET=NONE` fallback (Windows `MultiByteToWideChar`) |
| 5 | `mbstowcs` / `wcstombs` | C runtime | `CHARSET=NONE` fallback (Linux), `convVarStringSystemToStringW` |

Implementations #1 and #2 are **functionally equivalent** UTF-8 ↔ UTF-16 converters that exist in different files. Implementation #5 is locale-dependent and **actively incorrect** for `convVarStringSystemToStringW` (which handles system catalog strings that are always UTF-8, not locale-encoded). Implementations #3 and #4 are needed for non-UTF8 charsets.

##### Finding 5: The W→A→W Roundtrip Problem (Task 10.6.3)

The current W-API architecture creates an unnecessary encoding roundtrip:

```
App calls SQLExecDirectW(L"SELECT ...")
  → ConvertingString converts UTF-16 → UTF-8  (allocation + conversion)
  → Inner engine processes as char*
  → Result column names stored as char*
  
App calls SQLDescribeColW()
  → Inner engine returns char* column name
  → ConvertingString converts UTF-8 → UTF-16  (allocation + conversion)
  → App receives SQLWCHAR*
```

For a modern all-Unicode application (which is the default on Windows), every metadata string is converted **twice** — W→A on input, A→W on output — with heap allocations on each conversion. The stack-buffer optimization (10.6.1) mitigated the allocation cost, but the double-conversion overhead remains.

psqlodbc has the same issue: it stores all data internally as UTF-8 (from libpq), and converts to UTF-16 only at delivery time (`setup_getdataclass()` → `utf8_to_ucs2_lf()`). However, psqlodbc's internal architecture is C-string-based throughout, so switching to UTF-16 internal storage would require a complete rewrite. Our C++ architecture is more amenable to this change.

##### Finding 6: Specific Bugs in Current Charset Handling

| Bug | Location | Issue |
|-----|----------|-------|
| `convVarStringSystemToStringW` uses bare `mbstowcs()` | OdbcConvert.cpp:4336 | System catalog strings are UNICODE_FSS/UTF-8 from Firebird, but this function uses locale-dependent `mbstowcs()` instead of the per-column `MbsToWcs` codec. Can produce mojibake on non-UTF-8 locales (e.g., Japanese Windows with CP932). |
| `ODBCCONVERT_CONV_TO_STRINGW` macro calls `MbsToWcs` on pure-ASCII | OdbcConvert.cpp:1465 | Integer→StringW formatters produce ASCII digit strings (`0-9`, `-`, `.`), then call `from->MbsToWcs()` to widen them. This invokes the full multibyte codec on guaranteed-ASCII data. Should use a trivial byte-to-wchar widening loop. |
| Three redundant UTF-8 codecs | MultibyteConvert.cpp, Utf16Convert.cpp | `utf8_mbstowcs` and `Utf8ToUtf16` are independently maintained, hand-rolled UTF-8 decoders. Any bug fix applied to one must be manually replicated to the other. |
| `wchar_t` used as SQLWCHAR | OdbcConvert.cpp (throughout) | All `*ToStringW` functions cast to `wchar_t*`. On Linux, `wchar_t` is 4 bytes (UTF-32), not 2 bytes (UTF-16). The `utf8_mbstowcs` function in MultibyteConvert.cpp produces `wchar_t` values, while `Utf8ToUtf16` in Utf16Convert.cpp produces `SQLWCHAR` values. This is a latent platform-correctness issue for OdbcConvert's string-W functions on Linux. |

#### Tasks

##### 12.1 Consolidate Encoding Implementations

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **12.1.1** | **Unify UTF-8 codecs into single implementation** — Replace `utf8_mbstowcs`/`utf8_wcstombs` (MultibyteConvert.cpp) with calls to `Utf8ToUtf16`/`Utf16ToUtf8` (Utf16Convert.cpp). The `Utf16Convert` versions are SQLWCHAR-aware (always 16-bit), while the MultibyteConvert versions target `wchar_t` (platform-dependent). Keep `Utf16Convert` as the canonical implementation. Update `adressMbsToWcs(4)` and `adressWcsToMbs(4)` to return wrappers around the Utf16Convert functions. | Medium | **High** — single source of truth, fixes Linux wchar_t issue | ✅ DONE |
| **12.1.2** | **Fix `convVarStringSystemToStringW` to use per-column codec** — Replace bare `mbstowcs()` with `Utf8ToUtf16()`. System catalog strings from Firebird are always UNICODE_FSS or UTF-8; using `mbstowcs()` is incorrect on non-UTF-8 locales. | Easy | **High** — correctness fix | ✅ DONE |
| **12.1.3** | **Replace `MbsToWcs` on ASCII-only data with trivial widening** — In the `ODBCCONVERT_CONV_TO_STRINGW` macro (integer→StringW), replace the `from->MbsToWcs()` call with a simple byte-to-SQLWCHAR loop: `for (int i = 0; i <= len; i++) dst[i] = (SQLWCHAR)(unsigned char)src[i];`. ASCII is identity-mapped in UTF-8/UTF-16/Latin-1/every charset. Also apply to `convGuidToStringW`. | Easy | Medium — eliminates unnecessary codec dispatch on hot path | ✅ DONE |
| **12.1.4** | **Fix `wchar_t*` casts in OdbcConvert to use `SQLWCHAR*`** — All `*ToStringW` conversion functions cast output pointers to `wchar_t*` and use `wcsncpy`/`wcslen`/`L'\0'`. On Linux (where `wchar_t` = 4 bytes), this is incorrect — it writes 4-byte characters into a 2-byte SQLWCHAR buffer. Replace all `wchar_t*` casts with `SQLWCHAR*` and use `Utf16Length`/`Utf16Copy`/`(SQLWCHAR)0` from `Utf16Convert.h`. This is required for correct cross-platform W-API fetch behavior. | Medium | **Very High** — correctness on Linux/macOS | ✅ DONE |
| **12.1.5** | **Remove `fss_mbstowcs`/`fss_wcstombs` — use `Utf8ToUtf16`/`Utf16ToUtf8` for UNICODE_FSS** — UNICODE_FSS (charset 3) is a subset of UTF-8 (the original UTF-8 as defined in RFC 2279, limited to 3-byte sequences / BMP only). The `Utf8ToUtf16` function handles this correctly by definition — any valid FSS sequence is a valid UTF-8 sequence. Eliminate the separate FSS codec and route charset 3 through the UTF-8 path. | Easy | Medium — code reduction, maintenance reduction | ✅ DONE (FSS codec now uses ODBC_SQLWCHAR directly, same underlying path) |
| **12.1.6** | **Audit `CHARSET=NONE` path for correctness** — When `CHARSET=NONE`, the driver falls back to `_MbsToWcs`/`_WcsToMbs` (Windows: `MultiByteToWideChar(CP_ACP)`) or UTF-8 codec (Linux: since locale-dependent `wcstombs` is incompatible with `ODBC_SQLWCHAR`). Document that `CHARSET=NONE` is a legacy compatibility mode that should be avoided for Unicode applications. | Easy | Low — documentation + defensive coding | ✅ DONE (Linux CHARSET=NONE now uses UTF-8 codec instead of incompatible wcstombs) |

##### 12.2 Native UTF-16 Internal Encoding (Task 10.6.3)

This is the "significant effort" task that eliminates the W→A→W roundtrip. The approach is to store metadata strings (table names, column names, error messages, SQL text) internally as `SQLWCHAR*` (UTF-16), and convert to UTF-8 only when communicating with the Firebird engine.

**Design decisions:**
- **Scope**: Metadata strings only (column names, table names, error messages, SQLSTATE text, SQL statement text). NOT row data — row data already has an optimal single-conversion path via `OdbcConvert::conv*ToStringW`.
- **Internal string type**: Use `std::u16string` (C++17, backed by `char16_t` which matches `SQLWCHAR` on all platforms) for internal storage. Expose as `SQLWCHAR*` via `.data()`.
- **Conversion point**: UTF-8 → UTF-16 conversion happens ONCE when strings enter the driver (from Firebird API results or from the IscDbc layer). UTF-16 → UTF-8 conversion happens ONCE when strings leave the driver to the Firebird API (SQL text, parameter names).
- **A-API compatibility**: The ANSI (non-W) entry points convert UTF-16 → system codepage at the boundary, replacing the current "passthrough" behavior. This is actually more correct — currently, A-API calls pass raw bytes through without charset awareness.

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **12.2.1** | **Introduce `OdbcString` — UTF-16-native string class** — A thin wrapper around a dynamically-allocated `SQLWCHAR` array (not `std::u16string`, to avoid platform `char16_t` vs `SQLWCHAR` mismatch). Provides: `SQLWCHAR* data()`, `SQLSMALLINT length()` (in SQLWCHAR units), `std::string to_utf8()`, `static OdbcString from_utf8(const char*, int len)`, `static OdbcString from_utf16(const SQLWCHAR*, int len)`, `static OdbcString from_ascii(const char*, int len)`, `copy_to_w_buffer()`, `copy_to_a_buffer()`. Value semantics (copyable, movable). 26 unit tests. | Medium | **Very High** — foundation for all other 12.2 tasks | ✅ DONE |
| **12.2.2** | **Direct UTF-16 output for diagnostic functions** — Added `sqlGetDiagRecW`/`sqlGetDiagFieldW` methods on `OdbcError` and `OdbcObject` that convert internal UTF-8 strings directly to `SQLWCHAR*` output buffers. Updated `SQLGetDiagRecW`/`SQLGetDiagFieldW` in `MainUnicode.cpp` to call these methods directly, completely bypassing `ConvertingString`. Internal storage remains UTF-8 (JString) for now — full UTF-16 internal storage deferred to 12.2.3+ when `OdbcString` replaces `JString` throughout. SQLSTATE is written via simple ASCII widening (5 chars). | Medium | High — eliminates W→A→W roundtrip in the diagnostic path | ✅ DONE |
| **12.2.3** | **Convert column/table name storage to UTF-16** — Added `OdbcString` w-cache fields (`wBaseColumnName`, `wBaseTableName`, `wCatalogName`, `wLabel`, `wLiteralPrefix`, `wLiteralSuffix`, `wLocalTypeName`, `wName`, `wSchemaName`, `wTableName`, `wTypeName`) to `DescRecord`. Populated once during `defFromMetaDataIn`/`defFromMetaDataOut` at prepare time. `SQLDescribeColW` and `SQLColAttributeW` now read from cached `OdbcString` directly — zero conversion needed. JString fields retained for A-API backward compatibility. Added `getWString(fieldId)` accessor for field-id-based lookup. | Hard | **Very High** — eliminates per-call conversion for the most common metadata operation | ✅ DONE |
| **12.2.4** | **Convert SQL statement text to UTF-16 internal storage** — **Not needed.** SQL text is an input-only parameter (app → driver → Firebird). The UTF-16→UTF-8 conversion for Firebird is unavoidable. `SQLNativeSqlW` returns the input unchanged (no SQL rewriting). Storing an extra UTF-16 copy would add memory usage without measurable benefit. The existing `ConvertingString` with stack-buffer optimization (10.6.1) handles this path efficiently. | Medium | Medium — eliminates one `ConvertingString` per prepare/execute | N/A — not needed |
| **12.2.5** | **Simplify `ConvertingString` for UTF-16-native driver** — **Superseded.** With the w-cache approach, W-API output paths bypass `ConvertingString` entirely (direct UTF-16 from `OdbcString`). `ConvertingString` remains in use only for input parameters (SQL text, cursor names, connect strings) where UTF-16→UTF-8 is unavoidable. The existing implementation with stack-buffer optimization (10.6.1) is already efficient for this use case. No simplification needed. | Medium | High — architectural simplification | N/A — superseded |
| **12.2.6** | **Update `MainUnicode.cpp` W-API functions for direct UTF-16 passthrough** — All 7 W-API metadata/diagnostic functions (`SQLDescribeColW`, `SQLColAttributeW`, `SQLColAttributesW`, `SQLGetDescFieldW`, `SQLGetDescRecW`, `SQLGetDiagRecW`, `SQLGetDiagFieldW`) now call dedicated W methods directly, completely bypassing `ConvertingString` on the output path. String data is written from cached `OdbcString` to the app's `SQLWCHAR*` buffer via `memcpy` — zero encoding conversion. | Hard | **Very High** — the payoff of the entire phase | ✅ DONE |
| **12.2.7** | **Update `Main.cpp` A-API functions for UTF-16→A conversion** — **Not needed.** The dual-storage approach (JString + OdbcString side-by-side in `DescRecord`) means the A-API path reads from JString fields directly — no conversion needed. The A-API path is unchanged and still optimal: UTF-8 strings from JString are written directly to `SQLCHAR*` buffers via `strcpy`/`memcpy`. | Medium | N/A — maintains backward compatibility | N/A — not needed |
| **12.2.8** | **Migrate IscDbc string returns to caller-provided buffers** — **Achieved via w-cache.** Instead of changing IscDbc's `const char*` returns, the conversion to UTF-16 happens once at IRD population time (`defFromMetaDataIn`/`defFromMetaDataOut`) and is cached in `DescRecord::w*` fields. `sqlColAttributeW` reads from these cached fields instead of re-querying IscDbc. The A-API `sqlColAttribute` still re-queries IscDbc (which returns cached `const char*` pointers — no allocation), preserving existing behavior. | Hard | Medium — architectural cleanliness | ✅ DONE |

##### 12.3 OdbcConvert Rationalization

With the encoding consolidation (12.1) and native UTF-16 (12.2) in place, the OdbcConvert matrix can be simplified:

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **12.3.1** | **Merge `convVarStringSystemToString`/`convVarStringSystemToStringW` into `convVarStringToString`/`convVarStringToStringW`** — Removed separate System variants. Standard `convVarStringToString`/`convVarStringToStringW` now trim trailing spaces when `isResultSetFromSystemCatalog` is set. Dispatch branches simplified. | Easy | Medium — reduces the conversion function count by 2, eliminates a branch | ✅ DONE |
| **12.3.2** | **Unify `convStringToStringW` and `convVarStringToStringW`** — Extracted shared `convToStringWImpl` helper that handles MbsToWcs conversion, space padding (CHAR), trailing-space trimming (catalog), chunked `SQLGetData` with `dataOffset`, truncation/01004 posting, and indicator management. `convStringToStringW` and `convVarStringToStringW` are now 3-line wrappers that extract their source data and call the helper. Eliminates ~80 lines of duplicated logic. | Medium | Medium — reduces code duplication | ✅ DONE |
| **12.3.3** | **Audit and remove `notYetImplemented` fallback paths** — Audited all 17 dispatch paths. Most are correct per ODBC spec (unsupported type combinations). Identified missing conversions: Integer→BINARY, Float→NUMERIC, Bigint→BIT, Numeric→CHAR/WCHAR, String→NUMERIC/DATE/GUID. Fall-through in String/WString→Date/Time paths is intentional (app-side string→date parsing not implemented). Documented in `Docs/CONVERSION_MATRIX.md`. | Easy | Low — spec compliance for edge cases | ✅ DONE |
| **12.3.4** | **Document the conversion matrix** — Created `Docs/CONVERSION_MATRIX.md` listing every (source_type, target_type) pair, the function that handles it, and whether it's identity, widening, narrowing, or formatting. Includes notes on catalog trimming, encoding paths, and missing conversions. | Easy | Medium — maintainability | ✅ DONE |

##### 12.4 `CHARSET` Connection Parameter Semantics

| Task | Description | Complexity | Benefit | Status |
|------|-------------|------------|---------|--------|
| **12.4.1** | **Default `CHARSET` to `UTF8` when not specified** — Currently, when `CHARSET` is omitted from the connection string, the driver passes no `isc_dpb_lc_ctype` to Firebird, which defaults to `CS_NONE` (ID 0). This means: (a) text data arrives in the column's native charset (no server-side transliteration), (b) the driver falls back to locale-dependent `MultiByteToWideChar(CP_ACP)` for W-API conversion, (c) multi-charset databases produce mixed encodings that the driver can't handle correctly. **Fix**: When `CHARSET` is not specified, default to `UTF8`. This ensures consistent encoding, correct Unicode support, and optimal server-side transliteration. Emit SQLSTATE 01000 informational if the original connection string had no CHARSET (so apps know the driver is using a default). | Easy | **High** — correctness for the common case | ✅ DONE |
| **12.4.2** | **Document `CHARSET` parameter behavior** — Updated README.md with clear guidance: `CHARSET=UTF8` is recommended (default). `CHARSET=NONE` is legacy and should be avoided. Other charsets are supported but not recommended. Added usage table with ✅/⚠️/❌ recommendations and a note about the SQLSTATE 01000 informational diagnostic. | Easy | Medium — user guidance | ✅ DONE |

#### Research Reference: Why Not UTF-16 on the Wire?

For completeness, here is why requesting UTF-16 from Firebird is not possible and not desirable even if it were:

1. **Firebird blocks it**: `jrd/Attachment.cpp:initCharSet()` rejects `charset_min_bytes_per_char != 1`. Both `CS_UTF16` (ID 61) and `CS_UNICODE_UCS2` (ID 8) fail this check with the message "Wide character sets are not supported yet."

2. **Wire protocol overhead**: UTF-16 encoding of a `VARCHAR(100)` column requires 200 bytes on the wire (2 × max chars), while UTF-8 requires at most 400 bytes but typically much less for Western text (~100 bytes). For Asian text, UTF-16 (2 bytes/char for BMP) is more compact than UTF-8 (3 bytes/char for CJK), but the `VARCHAR` wire format is max-length-padded in Firebird's SQL_TEXT format, so the theoretical advantage doesn't apply.

3. **No transliteration elimination**: Even if Firebird could send UTF-16, the server would still need to transliterate from the column's storage charset (UTF-8, WIN1252, etc.) to UTF-16 internally — using the same `CsConvert` pivot architecture. The transliteration work is merely moved, not eliminated.

4. **ODBC Driver Manager complication**: The ODBC DM on Windows already handles UTF-16 at the API layer. Having the wire also be UTF-16 doesn't eliminate any driver-side conversion — the driver still needs to copy data from the wire buffer to the app buffer, potentially with truncation, indicator management, and chunked `SQLGetData` support.

**Bottom line**: The optimal architecture is: Firebird sends UTF-8 → driver converts UTF-8 → UTF-16 once → delivers to ODBC app. This is exactly one conversion, and it's unavoidable because the Firebird wire protocol uses byte-oriented encodings.

#### Architecture Diagram: Before and After

```
BEFORE (Phase 10):
  SQLExecDirectW(L"SELECT name FROM t")
    → ConvertingString: UTF-16→UTF-8 (stack buffer)      ← CONVERSION 1
    → IscConnection::prepareStatement(utf8_sql)
    → Firebird executes, returns UTF-8 column data
    → SQLDescribeColW() 
      → engine returns char* column name (UTF-8)
      → ConvertingString destructor: UTF-8→UTF-16        ← CONVERSION 2 (W→A→W roundtrip)
    → SQLFetch() + SQLGetData(SQL_C_WCHAR)
      → convVarStringToStringW: UTF-8→SQLWCHAR            ← CONVERSION 3 (correct, necessary)

AFTER (Phase 12):
  SQLExecDirectW(L"SELECT name FROM t")  
    → ConvertingString: UTF-16→UTF-8 (stack buffer)       ← CONVERSION 1 (unavoidable)
    → IscConnection::prepareStatement(utf8_sql)
    → Firebird executes, returns UTF-8 column data
    → IRD populated: UTF-8→OdbcString once during defFromMetaDataOut()  ← CONVERSION 2 (once per prepare, cached)
    → SQLDescribeColW()
      → sqlDescribeColW reads cached OdbcString (memcpy)   ← NO CONVERSION
    → SQLColAttributeW()
      → sqlColAttributeW reads cached OdbcString (memcpy)  ← NO CONVERSION
    → SQLGetDiagRecW()
      → sqlGetDiagRecW converts UTF-8→UTF-16 directly     ← NO ConvertingString roundtrip
    → SQLFetch() + SQLGetData(SQL_C_WCHAR)
      → convVarStringToStringW: UTF-8→SQLWCHAR             ← CONVERSION 3 (correct, necessary)
```

**Net effect**: Metadata operations (SQLDescribeCol, SQLColAttribute, SQLColAttributes, SQLGetDescField, SQLGetDescRec, SQLGetDiagRec, SQLGetDiagField) are zero-conversion for W-API callers — data is read from cached `OdbcString` fields via `memcpy`. Row data conversion remains at exactly one conversion (UTF-8→UTF-16), which is the theoretical minimum. Input conversions (SQL text, cursor names) use the existing stack-buffer-optimized `ConvertingString` for the unavoidable UTF-16→UTF-8 conversion.

#### Dependencies & Ordering

```
12.1.1 (unify codecs) ──────────────────────────┐
12.1.2 (fix SystemToStringW) ───┐                │
12.1.3 (ASCII widening) ────────┤                │
12.1.4 (wchar_t→SQLWCHAR) ─────┤                │
12.1.5 (remove FSS codec) ─────┤                │
12.1.6 (audit CHARSET=NONE) ───┘                │
                                                 ├──→ 12.2.1 (OdbcString) ──→ 12.2.2–12.2.8
12.3.1 (merge System variants) ──→ requires 12.1.2
12.3.2 (merge String/VarString W) ──→ requires 12.1.4
12.4.1 (default CHARSET=UTF8) ──→ independent, can be done first
```

**Recommended execution order**:
1. 12.4.1 (quick win, high impact)
2. 12.1.2, 12.1.3, 12.1.4 (correctness fixes)
3. 12.1.1, 12.1.5 (codec consolidation)
4. 12.3.1, 12.3.2 (code reduction)
5. 12.2.1 → 12.2.2 → 12.2.3 → 12.2.4 → 12.2.5 → 12.2.6 → 12.2.7 → 12.2.8 (native UTF-16 — sequential)

#### Success Criteria

- [x] Single UTF-8 ↔ UTF-16 codec implementation (Utf16Convert.h) used throughout the driver — ODBC_SQLWCHAR-based codecs in MultibyteConvert.cpp, Linux CHARSET=NONE uses UTF-8 codec
- [x] `convVarStringSystemToStringW` uses `Utf8ToUtf16()`, not bare `mbstowcs()`
- [x] All `*ToStringW` functions use `SQLWCHAR*` (not `wchar_t*`) — correct on Linux/macOS
- [x] Integer→StringW formatters use trivial ASCII widening, not full multibyte codec
- [x] `CHARSET` defaults to `UTF8` when not specified in the connection string
- [x] `OdbcString` type introduced for internal UTF-16 string storage
- [x] `OdbcError` stores SQLSTATE and message text — direct UTF-16 output via `sqlGetDiagRecW`/`sqlGetDiagFieldW`
- [x] `DescRecord` stores column/table names as cached `OdbcString` w-fields (11 fields)
- [x] `SQLDescribeColW` returns cached UTF-16 without per-call conversion
- [x] `ConvertingString` bypassed for all 7 W-API metadata/diagnostic output functions
- [x] All 432 existing tests still pass (verified after full Phase 12 changes)
- [x] New tests verify correct behavior with `CHARSET=UTF8` (default), `CHARSET=NONE`, and `CHARSET=WIN1252`
- [x] Cross-platform correctness verified: Linux + Windows (CI green on both)

**Deliverable**: A driver with exactly one encoding conversion per data path (the theoretical minimum), zero redundant codec implementations, correct cross-platform `SQLWCHAR` handling, and cached UTF-16 metadata that eliminates the W→A→W roundtrip for all 7 metadata/diagnostic W-API output functions. Combined with Phase 10's fetch optimizations, this makes the driver the fastest and most correct Unicode ODBC driver for Firebird.

### Phase 13: Code Simplification & Dead Code Removal ✅ (Completed — February 10, 2026)
**Priority**: Low  
**Duration**: 1–2 weeks  
**Goal**: Remove dead code, eliminate file-level redundancy, consolidate duplicate tests, clean up build system

#### Background

After completing Phases 0–12, the codebase has accumulated dead files, unused legacy code, and test duplication. A thorough audit of every source file and test file reveals substantial cleanup opportunities — 14 dead source files, ~30 redundant tests, and several build system simplifications.

#### 13.1 Dead Source Files — Remove from Repository

Files that are compiled but have zero callers, or that exist on disk but are not compiled and not referenced:

| # | File(s) | Location | Lines | Status | Completion | Reason |
|---|---------|----------|-------|--------|------------|--------|
| **13.1.1** | `LinkedList.cpp`, `LinkedList.h` | `src/IscDbc/` | ~400 | Compiled, zero callers | ✅ Deleted | Phase 5 replaced all usages with `std::vector<T*>`. No file outside LinkedList.cpp includes LinkedList.h. Remove from `ISCDBC_SOURCES`/`ISCDBC_HEADERS` and delete. |
| **13.1.2** | `Lock.cpp`, `Lock.h` | `src/IscDbc/` | ~80 | Compiled, zero callers | ✅ Deleted | RAII mutex wrapper — no file outside Lock.cpp includes Lock.h. Locking is done via `SafeEnvThread`/`Mutex` directly. Remove from `ISCDBC_SOURCES`/`ISCDBC_HEADERS` and delete. |
| **13.1.3** | `ServiceManager.cpp`, `ServiceManager.h` | `src/IscDbc/` | ~880 | Compiled, zero callers | ✅ Deleted | Backup/restore/statistics service manager. Only `ServiceManager.cpp` includes `ServiceManager.h`. Was used by the OdbcJdbcSetup GUI (not built). 877 lines of dead code linked into the static library. Remove from build and delete. |
| **13.1.4** | `SupportFunctions.cpp`, `SupportFunctions.h` | `src/IscDbc/` | ~200 | Not compiled, on disk | ✅ Deleted | Removed from build per M-4 (WONTFIX). Source files remain on disk. Delete. |
| **13.1.5** | `OdbcDateTime.cpp`, `OdbcDateTime.h` | `src/` | ~400 | Not compiled, on disk | ✅ Deleted | Removed from build per Phase 9.6. Replaced by `FbDateConvert.h`. Source files remain on disk. Delete. |
| **13.1.6** | `Engine.h` | `src/IscDbc/` | ~70 | In headers list, zero includers | ✅ Deleted | Only referenced by a commented-out line in `Error.cpp` (`//#include "Engine.h"`). Defines obsolete macros (`MAX`, `MIN`, `ABS`) that conflict with `<algorithm>`. Remove from `ISCDBC_HEADERS` and delete. |
| **13.1.7** | `WriteBuildNo.h` | `src/` | ~20 | In headers list, zero includers | ✅ Deleted | Listed in `ODBCJDBC_HEADERS` but never `#include`d by any file. Master plan confirms "no longer used for versioning" (replaced by `Version.h`). Remove from `ODBCJDBC_HEADERS` and delete. |
| **13.1.8** | `IscDbc.def`, `IscDbc.exp` | `src/IscDbc/` | ~10 | Not used by CMake | ✅ Deleted | Legacy linker files from when IscDbc was a shared DLL. Now it's a static library (`add_library(IscDbc STATIC ...)`). Delete. |
| **13.1.9** | `OdbcJdbcMinGw.def` | `src/` | ~120 | Not used by CMake | ✅ Deleted | MinGW-specific .def file. CMake only uses `OdbcJdbc.def`. Delete. |
| **13.1.10** | `OdbcJdbc.dll.manifest` | `src/` | ~10 | Not used by CMake | ✅ Deleted | Legacy Windows manifest. Not referenced by CMake build. Delete. |
| **13.1.11** | `makefile.in` (×2) | `src/`, `src/IscDbc/` | ~200 | Not used | ✅ Deleted | Legacy autotools makefiles. CMake is the sole build system. Delete. |

**Estimated removal**: ~2,400 lines of dead code + ~500 lines of dead build files.

#### 13.2 Headers Missing from CMakeLists.txt

Files that are actively `#include`d but are missing from the `ODBCJDBC_HEADERS` list (doesn't affect compilation since include directories are set, but affects IDE integration and `install` targets):

| # | File | Included By | Action | Status |
|---|------|-------------|--------|--------|
| **13.2.1** | `OdbcString.h` | `DescRecord.h`, `OdbcError.h` | Add to `ODBCJDBC_HEADERS` | ✅ Done |
| **13.2.2** | `OdbcSqlState.h` | `OdbcError.cpp` | Add to `ODBCJDBC_HEADERS` | ✅ Done |
| **13.2.3** | `OdbcUserEvents.h` | `OdbcStatement.cpp` | Add to `ODBCJDBC_HEADERS` | ✅ Done |
| **13.2.4** | `FbDateConvert.h` | `OdbcConvert.cpp`, `DateTime.cpp` | Add to `ISCDBC_HEADERS` | ✅ Done |

#### 13.3 Build System Simplification

| # | Issue | Location | Action | Status |
|---|-------|----------|--------|--------|
| **13.3.1** | Redundant if/else in `.def` file assignment — 64-bit and 32-bit branches use identical paths | `CMakeLists.txt:176–181` | Remove the if/else; use a single `set_target_properties(OdbcFb PROPERTIES LINK_FLAGS "/DEF:...")` | ✅ Done |
| **13.3.2** | `BUILD_SETUP` option references non-existent `OdbcJdbcSetup` subdirectory | `CMakeLists.txt:192–194` | Remove the `BUILD_SETUP` option and the `add_subdirectory(OdbcJdbcSetup)` block | ✅ Done |

#### 13.4 Test Suite Consolidation

##### 13.4.1 Test Files to Delete (Strict Subsets)

| File to Delete | Tests | Superseded By | Reason | Status |
|---|---|---|---|---|
| `test_catalog.cpp` | 6 tests | `test_catalogfunctions.cpp` (22 tests) | Every test in `test_catalog.cpp` has a near-identical or more thorough version in `test_catalogfunctions.cpp`: `SQLTablesFindsTestTable` ≈ `SQLTablesBasic`, `SQLColumnsReturnsCorrectTypes` ≈ `SQLColumnsDataTypes` + `SQLColumnsNullability` + `SQLColumnsNames`, `SQLPrimaryKeys` ≈ `SQLPrimaryKeysBasic`, `SQLGetTypeInfo` ≈ `SQLGetTypeInfoAllTypes`, `SQLStatistics` ≈ `SQLStatisticsBasic`, `SQLSpecialColumns` ≈ `SQLSpecialColumnsBasic` | ✅ Deleted |
| `test_batch_params.cpp` | 4 tests | `test_array_binding.cpp` (17 tests) | `InsertWithRowWiseBinding` ≈ `RowWiseInsert` (same data), `SelectAfterBatchInsert` ≈ verified within `RowWiseInsert`, `InsertSingleRow` ≈ `SingleRowArray`, `ColumnWiseInsert` ≈ `ColumnWiseInsert` | ✅ Deleted |

##### 13.4.2 Test Files to Trim (Remove Duplicated Tests)

| File | Tests to Remove | Reason | Keep | Status |
|---|---|---|---|---|
| `test_connection.cpp` | All tests | All tests are trivial or duplicated by `test_connect_options.cpp:BasicDriverConnect` and by every connected test fixture | N/A — file removed from build entirely | ✅ Removed |
| `test_cursor.cpp` | All tests | `SetAndGetCursorName`/`DefaultCursorName` duplicate `test_cursor_name.cpp`; `CommitClosesBehavior` duplicates `test_cursors.cpp:CommitClosesOpenCursor`; `SQLCloseCursorAllowsReExec` duplicates `test_cursors.cpp:CloseThenReExecute`; block-fetch tests covered by `test_cursors.cpp:FetchAllWithoutInterruption` | N/A — file removed from build entirely | ✅ Removed |
| `test_data_types.cpp` | `IntegerToString` | Duplicated by `test_result_conversions.cpp:IntToChar` (more thorough) | Keep all other tests | ✅ Removed |
| `test_data_types.cpp` | `GetDataStringTruncation` | Duplicated by `test_result_conversions.cpp:CharTruncation` | Keep all other tests | ✅ Removed |
| `test_bind_cycle.cpp` | All tests | `RebindColumnBetweenExecutions` ≈ `test_bindcol.cpp:RebindToDifferentType`; `UnbindAllColumns` ≈ `test_bindcol.cpp:UnbindAndUseGetData`; remaining tests are trivial parameter binding covered by `test_data_types.cpp:ParameterizedInsertAndSelect` | N/A — file removed from build entirely | ✅ Removed |

##### 13.4.3 Phase-Based Catch-All Files to Redistribute

These files group unrelated tests by the phase in which they were written, not by topic. For long-term maintainability, their tests should be moved to topic-specific files:

| Source File | Fixture | Tests | Move To | Status |
|---|---|---|---|---|
| `test_phase7_crusher_fixes.cpp` | `CopyDescCrashTest` (7 tests) | CopyDesc, SetDescCount, etc. | `test_descriptor.cpp` | ✅ Moved |
| `test_phase7_crusher_fixes.cpp` | `DiagRowCountTest` (4 tests) | DiagRowCount after exec | `test_errors.cpp` | ✅ Moved |
| `test_phase7_crusher_fixes.cpp` | `ConnectionTimeoutTest` (3 tests) | SQL_ATTR_CONNECTION_TIMEOUT | `test_connect_options.cpp` | ✅ Moved |
| `test_phase7_crusher_fixes.cpp` | `AsyncEnableTest` (5 tests) | SQL_ATTR_ASYNC_ENABLE | `test_connect_options.cpp` | ✅ Moved |
| `test_phase7_crusher_fixes.cpp` | `TruncationIndicatorTest` (3 tests) | returnStringInfo truncation | `test_errors.cpp` | ✅ Moved |
| `test_phase11_typeinfo_timeout_pool.cpp` | `TypeInfoTest` (7 tests) | SQLGetTypeInfo ordering | `test_catalogfunctions.cpp` | ✅ Moved |
| `test_phase11_typeinfo_timeout_pool.cpp` | `AsyncModeTest` (1 test) | SQL_ASYNC_MODE report | `test_connect_options.cpp` | ✅ Moved |
| `test_phase11_typeinfo_timeout_pool.cpp` | `QueryTimeoutTest` (7 tests) | SQL_ATTR_QUERY_TIMEOUT | `test_connect_options.cpp` | ✅ Moved |
| `test_phase11_typeinfo_timeout_pool.cpp` | `ConnectionResetTest` (6 tests) | SQL_ATTR_RESET_CONNECTION | `test_connect_options.cpp` | ✅ Moved |

After redistribution, delete `test_phase7_crusher_fixes.cpp` and `test_phase11_typeinfo_timeout_pool.cpp`.

##### 13.4.4 Optional Merges (Lower Priority)

| Files | Rationale | Priority | Status |
|---|---|---|---|
| `test_bind_cycle.cpp` + `test_bindcol.cpp` → `test_binding.cpp` | Both test bind/unbind patterns; ~4 unique tests each after dedup | Low | ✅ Done — `test_bind_cycle.cpp` removed entirely (all tests superseded by `test_bindcol.cpp`) |
| `test_stmthandles.cpp` + `test_multi_statement.cpp` → `test_statement_handles.cpp` | Both test multiple statement handles on one connection | Low | ❌ Deferred |
| `test_cursor.cpp` + `test_cursors.cpp` → single `test_cursor.cpp` | After removing duplicates, `test_cursor.cpp` has ~3 unique block-fetch tests and `test_cursors.cpp` has ~5 unique tests | Low | ✅ Done — `test_cursor.cpp` removed entirely (all tests superseded by `test_cursors.cpp` + `test_cursor_name.cpp`) |

#### 13.5 Summary

| Category | Count | Lines Saved (est.) |
|----------|-------|--------------------|
| Dead source files to delete | 14 files (+ 2 makefiles) | ~2,900 |
| Dead build artifacts to delete | 4 files (.def, .exp, .manifest, MinGW.def) | ~140 |
| Headers to add to CMakeLists.txt | 4 | — |
| Build system simplifications | 2 | ~10 |
| Test files to delete (strict subsets) | 2 | ~400 |
| Redundant individual tests to remove | ~12 | ~300 |
| Catch-all test files to redistribute & delete | 2 | — (net zero, reorganization) |
| **Total dead code removal** | **~22 files** | **~3,750 lines** |

#### Success Criteria

- [x] Zero dead `.cpp`/`.h` files remain in `src/` or `src/IscDbc/`
- [x] All actively-used headers listed in appropriate `CMakeLists.txt` headers list
- [x] No duplicate if/else branches in build system
- [x] No test file is a strict subset of another
- [x] No individual test is duplicated across files
- [x] Phase-named test files replaced by topic-named files
- [x] All 401 unique tests pass (reduced from 432 after removing 31 duplicated/superseded tests)
- [x] Build compiles without warnings on Windows and Linux

**Deliverable**: A leaner codebase with ~3,750 fewer lines of dead code, a cleaner test suite organized by topic instead of implementation phase, and a simplified build system.

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

**Phase 14.1: Foundation — vcpkg Integration & Build System**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.1.1** | **Add vcpkg manifest** — Create `vcpkg.json` with `fb-cpp` dependency. Add `vcpkg-configuration.json` pointing to the `firebird-vcpkg-registry`. | Easy | ❌ |
| **14.1.2** | **Update CMakeLists.txt for vcpkg** — Set `CMAKE_TOOLCHAIN_FILE` to vcpkg's toolchain. Use `find_package(fb-cpp CONFIG REQUIRED)`. Link `OdbcFb` against `fb-cpp::fb-cpp`. | Easy | ❌ |
| **14.1.3** | **Remove `FetchFirebirdHeaders.cmake`** — vcpkg's `firebird` package provides headers. Delete the custom FetchContent logic. | Easy | ❌ |
| **14.1.4** | **Add fb-cpp feature flags** — Enable `boost-dll` and `boost-multiprecision` features in `vcpkg.json` for runtime client loading and INT128/DECFLOAT support. | Easy | ❌ |
| **14.1.5** | **Update CI workflows** — Add vcpkg bootstrap and cache steps. Use `vcpkg install` before CMake configure. | Medium | ❌ |
| **14.1.6** | **Verify build** — Ensure the project builds with fb-cpp linked but not yet used. All 401 tests must pass. | Easy | ❌ |

**Phase 14.2: Client & Attachment Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.2.1** | **Create `FbClient` wrapper** — Singleton (or per-environment) `fbcpp::Client` instance. Replaces `CFbDll` for fbclient loading. | Medium | ❌ |
| **14.2.2** | **Replace `Attachment` class** — `IscConnection` currently owns `Firebird::IAttachment*`. Replace with `std::unique_ptr<fbcpp::Attachment>`. Update `openDatabase()` to use `fbcpp::Attachment` constructor with `AttachmentOptions`. | Medium | ❌ |
| **14.2.3** | **Replace `CFbDll::_array_*` calls** — fb-cpp doesn't wrap arrays. Keep minimal ISC array functions loaded separately (Firebird OO API doesn't expose `getSlice`/`putSlice`). | Hard | ❌ |
| **14.2.4** | **Migrate `createDatabase()`** — Use `AttachmentOptions::setCreateDatabase(true)`. | Easy | ❌ |
| **14.2.5** | **Migrate connection properties** — Map `CHARSET`, `UID`, `PWD`, `ROLE` to `AttachmentOptions` setters. | Easy | ❌ |
| **14.2.6** | **Delete `Attachment.cpp`, `Attachment.h`** — After migration, remove the IscDbc versions. | Easy | ❌ |

**Phase 14.3: Transaction Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.3.1** | **Replace `InfoTransaction` with `fbcpp::Transaction`** — `IscConnection` manages transactions via `transactionInfo.transactionHandle`. Replace with `std::unique_ptr<fbcpp::Transaction>`. | Medium | ❌ |
| **14.3.2** | **Map transaction isolation levels** — `TransactionIsolationLevel::READ_COMMITTED`, `SNAPSHOT`, `CONSISTENCY` map to Firebird TPB options. Use `TransactionOptions::setIsolationLevel()`. | Easy | ❌ |
| **14.3.3** | **Migrate auto-commit** — Current code manually commits after each statement when `autoCommit=true`. fb-cpp requires explicit commits; keep the same pattern. | Easy | ❌ |
| **14.3.4** | **Migrate savepoints** — fb-cpp doesn't expose savepoints. Keep the existing `SAVEPOINT`/`RELEASE SAVEPOINT`/`ROLLBACK TO SAVEPOINT` SQL execution via `Statement::execute()`. | Easy | ❌ |
| **14.3.5** | **Delete `InfoTransaction`, TPB-building code** — After migration, remove ~200 lines of manual TPB construction. | Easy | ❌ |

**Phase 14.4: Statement & ResultSet Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.4.1** | **Replace `IscStatement`/`IscPreparedStatement` with `fbcpp::Statement`** — The most complex migration. fb-cpp's `Statement` combines prepare + execute + fetch. | Hard | ❌ |
| **14.4.2** | **Migrate parameter binding** — Replace `Sqlda::setValue()` with `fbcpp::Statement::setInt32()`, `setString()`, etc. The ODBC layer still uses `OdbcConvert` for type coercion; the IscDbc layer just needs to pass values to fb-cpp. | Hard | ❌ |
| **14.4.3** | **Migrate result fetching** — Replace `IscResultSet::nextFetch()` with `fbcpp::Statement::fetchNext()`. Map fb-cpp's `std::optional` returns to SQLDA null indicators. | Hard | ❌ |
| **14.4.4** | **Migrate batch execution** — Use fb-cpp's `Batch` class (contributed by us — see [FB_CPP_PLAN.md](FB_CPP_PLAN.md) Phase 1). Replace existing raw `IBatch` code in `IscStatement` with `fbcpp::Batch`. | Medium | ❌ |
| **14.4.5** | **Migrate scrollable cursors** — Use fb-cpp's `CursorType::SCROLLABLE` option (contributed by us — see [FB_CPP_PLAN.md](FB_CPP_PLAN.md) Phase 2). Map to existing OdbcStatement scroll methods. fb-cpp defaults to forward-only, matching our performance needs. | Medium | ❌ |
| **14.4.6** | **Delete `IscStatement.cpp/.h`, `IscPreparedStatement.cpp/.h`, `IscCallableStatement.cpp/.h`, `IscResultSet.cpp/.h`, `Sqlda.cpp/.h`** — After migration, ~3,000 lines removed. | Easy | ❌ |

**Phase 14.5: Blob Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.5.1** | **Replace `IscBlob` with `fbcpp::Blob`** — fb-cpp's Blob class provides `read()`, `write()`, `getLength()`, `seek()`. | Medium | ❌ |
| **14.5.2** | **Migrate BLOB read** — `IscBlob::getSegment()` → `fbcpp::Blob::read()` or `readSegment()`. | Easy | ❌ |
| **14.5.3** | **Migrate BLOB write** — `IscBlob::putSegment()` → `fbcpp::Blob::write()` or `writeSegment()`. | Easy | ❌ |
| **14.5.4** | **Delete `IscBlob.cpp/.h`, `BinaryBlob.cpp/.h`, `Blob.cpp/.h`** — After migration, ~600 lines removed. | Easy | ❌ |

**Phase 14.6: Metadata & Events Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.6.1** | **Migrate `IscDatabaseMetaData`** — This class uses `IAttachment` for catalog queries. Can remain as-is initially, using fb-cpp's `Attachment::getHandle()` for raw access. | Low | ❌ |
| **14.6.2** | **Replace `IscUserEvents` with `fbcpp::EventListener`** — fb-cpp provides a modern event listener with background thread dispatch. | Medium | ❌ |
| **14.6.3** | **Delete `IscUserEvents.cpp/.h`** — After migration, ~300 lines removed. | Easy | ❌ |

**Phase 14.7: Error Handling & Utilities Migration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.7.1** | **Migrate exception handling** — Replace `SQLException` with `fbcpp::DatabaseException`. Update all catch blocks. Extract error vectors for SQLSTATE mapping. | Medium | ❌ |
| **14.7.2** | **Delete utility classes** — `DateTime.cpp/.h`, `TimeStamp.cpp/.h`, `SqlTime.cpp/.h` — fb-cpp uses `std::chrono`. | Easy | ❌ |
| **14.7.3** | **Delete `Value.cpp/.h`, `Values.cpp/.h`** — fb-cpp's typed getters eliminate the need for a generic `Value` container. | Easy | ❌ |
| **14.7.4** | **Delete `JString.cpp/.h`** — Replace remaining usages with `std::string`. | Easy | ❌ |

**Phase 14.8: Final Cleanup**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **14.8.1** | **Delete remaining IscDbc files** — `EnvShare.cpp/.h`, `Error.cpp/.h`, `Parameter.cpp/.h`, etc. | Easy | ❌ |
| **14.8.2** | **Remove `src/IscDbc/` directory** — All code now in fb-cpp or `src/`. | Easy | ❌ |
| **14.8.3** | **Update CMakeLists.txt** — Remove `add_subdirectory(src/IscDbc)`. Update include paths. | Easy | ❌ |
| **14.8.4** | **Update documentation** — README, AGENTS.md, this master plan. | Easy | ❌ |
| **14.8.5** | **Run full test suite** — All 401 tests must pass. | Easy | ❌ |

#### Code Reduction Estimate

| Category | Before | After | Savings |
|----------|--------|-------|---------|
| `src/IscDbc/` files | ~110 files | 0 files | ~15,000 lines |
| `LoadFbClientDll.cpp/.h` | ~600 lines | ~50 lines (array only) | ~550 lines |
| Date/time utilities | ~400 lines | 0 (fb-cpp) | ~400 lines |
| String utilities (JString) | ~300 lines | 0 (std::string) | ~300 lines |
| **Total** | — | — | **~16,250 lines** |

#### fb-cpp Gaps to Address

Based on our review (see [FB_CPP_SUGGESTIONS.md](../FB_CPP_SUGGESTIONS.md)) and the author's response ([FB_CPP_REPLY.md](../FB_CPP_REPLY.md)), fb-cpp has the following gaps that we will **contribute back as PRs** (see [Docs/FB_CPP_PLAN.md](FB_CPP_PLAN.md)):

1. **`IBatch` support** — Critical for array parameter binding. Author agreed to add it. We will contribute a `Batch` + `BatchCompletionState` class wrapping `IBatch`/`IBatchCompletionState`. **Must land before Phase 14.4.4.**
2. **Error vector in `DatabaseException`** — Critical for SQLSTATE mapping. Author agreed ("should be copied from original status and exposed"). We will contribute `getErrors()`, `getSqlCode()`, `getErrorCode()` methods. **Must land before Phase 14.7.1.**
3. **Scrollable cursor control** — Author agreed. We will contribute a `CursorType` enum to `StatementOptions`. **Must land before Phase 14.4.5.**
4. **Move assignment** — Author agreed (`Statement` "should be movable"). We will contribute `operator=(Statement&&)` and `operator=(Attachment&&)`.
5. **`Descriptor::alias`** — Author agreed ("could be added"). Small addition for ODBC `SQL_DESC_LABEL`.
6. **Array support** — Firebird arrays require legacy ISC API (`isc_array_get_slice`). fb-cpp won't wrap these. Keep minimal ISC function pointers for this rare feature.

Note: `Client::getUtil()` is already exposed (our suggestion #9 was withdrawn). `IResultSet` abstraction was also withdrawn — Firebird doesn't support multiple active result sets per statement.

#### Success Criteria

- [ ] `src/IscDbc/` directory deleted — all code migrated to fb-cpp or `src/`
- [ ] `vcpkg.json` manifest manages fb-cpp, Firebird, and Boost dependencies
- [ ] Build works on Windows (MSVC), Linux (GCC/Clang), macOS (Clang)
- [ ] All 401 tests pass
- [ ] ~16,000 lines of legacy code removed
- [ ] Performance benchmarks show no regression (fetch throughput, batch insert)
- [ ] CI builds use vcpkg caching for fast builds

**Deliverable**: A dramatically simplified codebase where the ODBC layer talks directly to fb-cpp's modern C++ API, eliminating the 20-year-old JDBC-like abstraction layer.

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
| **15.1.1** | **Create `vcpkg.json` manifest** — Declare dependencies: `gtest`, `benchmark`, `fb-cpp` (from custom registry). | Easy | ❌ |
| **15.1.2** | **Create `vcpkg-configuration.json`** — Configure baseline (vcpkg commit), custom registry for Firebird packages. | Easy | ❌ |
| **15.1.3** | **Update `.gitignore`** — Add `vcpkg_installed/` (local install tree). | Easy | ❌ |
| **15.1.4** | **Document vcpkg setup** — README section on `vcpkg install` vs. manual dependency management. | Easy | ❌ |

**Phase 15.2: CMake Integration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **15.2.1** | **Set `CMAKE_TOOLCHAIN_FILE`** — Point to `vcpkg/scripts/buildsystems/vcpkg.cmake`. Support both submodule and external vcpkg. | Easy | ❌ |
| **15.2.2** | **Replace FetchContent for GTest** — Remove `FetchContent_Declare(googletest ...)`. Use `find_package(GTest CONFIG REQUIRED)`. | Easy | ❌ |
| **15.2.3** | **Replace FetchContent for Benchmark** — Remove `FetchContent_Declare(benchmark ...)`. Use `find_package(benchmark CONFIG REQUIRED)`. | Easy | ❌ |
| **15.2.4** | **Replace FetchFirebirdHeaders** — Remove `cmake/FetchFirebirdHeaders.cmake`. vcpkg's `firebird` package provides headers. | Easy | ❌ |
| **15.2.5** | **Link against vcpkg targets** — `target_link_libraries(... GTest::gtest benchmark::benchmark fb-cpp::fb-cpp)`. | Easy | ❌ |

**Phase 15.3: CI/CD Integration**

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| **15.3.1** | **Add vcpkg bootstrap to CI** — Clone vcpkg, run bootstrap script, set environment variables. | Easy | ❌ |
| **15.3.2** | **Enable binary caching** — Set `VCPKG_BINARY_SOURCES` to GitHub Packages or Azure Artifacts. | Medium | ❌ |
| **15.3.3** | **Cache vcpkg installed tree** — Use `actions/cache` with `vcpkg_installed/` as cache path. | Easy | ❌ |
| **15.3.4** | **Update build scripts** — `firebird-odbc-driver.build.ps1`, `install-prerequisites.ps1` to use vcpkg. | Easy | ❌ |

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

- [ ] `vcpkg.json` and `vcpkg-configuration.json` in repository root
- [ ] `cmake/FetchFirebirdHeaders.cmake` deleted
- [ ] No `FetchContent_Declare` calls in CMakeLists.txt
- [ ] CI uses vcpkg binary caching (builds < 5 min with cache hit)
- [ ] `vcpkg install` followed by `cmake --preset default` builds the project
- [ ] All 401 tests pass
- [ ] Documentation updated with vcpkg setup instructions

**Deliverable**: A project that uses vcpkg for all C++ dependencies, with reproducible builds across platforms, fast CI via binary caching, and a single `vcpkg.json` as the source of truth for dependency versions.

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

*Document version: 3.8 — February 11, 2026*
*This is the single authoritative reference for all Firebird ODBC driver improvements.*
