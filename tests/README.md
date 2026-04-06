# Firebird ODBC Driver — Test Suite

Google Test-based test suite for the Firebird ODBC driver. Tests the driver through the **standard ODBC API** via the Driver Manager, connecting to a real Firebird database.

## Prerequisites

1. **Firebird Server** — a running Firebird 3.0+ instance with a test database  
2. **Firebird ODBC Driver** — pre-built and registered as an ODBC data source (or referenced via a `Driver=` connection string)  
3. **CMake** ≥ 3.14  
4. **C++17 compiler** (MSVC 2022, GCC 9+, Clang 10+)
5. **Google Test** — installed and discoverable via `find_package(GTest CONFIG REQUIRED)`

### Windows-specific

- The driver DLL (built via `Invoke-Build install`) must be registered as an ODBC driver. Use `Invoke-Build install` to register, or manually via `odbcconf`.
- Google Test can be installed via [vcpkg](https://vcpkg.io/): `vcpkg install gtest:x64-windows`, then pass `-DCMAKE_TOOLCHAIN_FILE=.../vcpkg/scripts/buildsystems/vcpkg.cmake` to CMake. Or build from source and pass `-DCMAKE_PREFIX_PATH=<gtest-install-dir>`.

### Linux-specific

- `unixODBC` development headers (`unixodbc-dev` / `unixODBC-devel`)  
- The driver `.so` must be registered in `/etc/odbcinst.ini`
- Google Test can be installed via your package manager: `apt install libgtest-dev` / `dnf install gtest-devel`

## Building the tests

The test suite is a standalone CMake project. Google Test and ODBC are found via `find_package()`.

```bash
# From the repository root:
cmake -B build-tests -S tests
cmake --build build-tests --config Release
```

## Running the tests

Set the `FIREBIRD_ODBC_CONNECTION` environment variable with a valid ODBC connection string, then run with CTest:

### Windows (PowerShell)

```powershell
$env:FIREBIRD_ODBC_CONNECTION = "Driver={Firebird ODBC Driver};Dbname=localhost:C:\path\to\test.fdb;Uid=SYSDBA;Pwd=masterkey;"
ctest --test-dir build-tests --output-on-failure -C Release
```

### Windows (cmd.exe)

```cmd
set FIREBIRD_ODBC_CONNECTION=Driver={Firebird ODBC Driver};Dbname=localhost:C:\path\to\test.fdb;Uid=SYSDBA;Pwd=masterkey;
ctest --test-dir build-tests --output-on-failure -C Release
```

### Linux

```bash
export FIREBIRD_ODBC_CONNECTION="Driver=Firebird;Dbname=localhost:/path/to/test.fdb;Uid=SYSDBA;Pwd=masterkey;"
ctest --test-dir build-tests --output-on-failure
```

### Running the test executable directly

```bash
./build-tests/firebird_odbc_tests --gtest_output=xml:test-results.xml
```

## Test categories

Tests are organized into three categories based on their expected behavior against the **current upstream driver** (vanilla master):

### Category A — Pass (~166 tests)

Standard ODBC functionality that the current driver supports. These tests are expected to **pass** on vanilla master.

| File | Tests | Description |
|------|-------|-------------|
| `test_data_types.cpp` | ~18 | SMALLINT, INTEGER, BIGINT, FLOAT, DOUBLE, NUMERIC, VARCHAR, DATE, TIME, TIMESTAMP |
| `test_result_conversions.cpp` | ~35 | SQLGetData type conversions |
| `test_param_conversions.cpp` | ~18 | SQLBindParameter type conversions |
| `test_prepare.cpp` | ~10 | SQLPrepare / SQLExecute lifecycle |
| `test_cursors.cpp` | ~7 | Cursor behavior, commit/rollback, close/re-execute |
| `test_cursor_commit.cpp` | ~6 | Cursor behavior across transactions |
| `test_cursor_name.cpp` | ~9 | SQLSetCursorName / SQLGetCursorName |
| `test_data_at_execution.cpp` | ~6 | SQL_DATA_AT_EXEC / SQLPutData |
| `test_array_binding.cpp` | ~17 | Column-wise + row-wise parameter arrays |
| `test_bindcol.cpp` | ~5 | Dynamic unbind/rebind mid-fetch |
| `test_descrec.cpp` | ~10 | SQLGetDescRec for all column types |
| `test_blob.cpp` | ~3 | Small/large/null BLOB read/write |
| `test_multi_statement.cpp` | ~4 | Multiple statement handles on one connection |
| `test_stmthandles.cpp` | ~4 | 100+ simultaneous statement handles |
| `test_wchar.cpp` | ~8 | SQL_C_WCHAR bind/fetch, truncation |
| `test_escape_sequences.cpp` | ~6 | Escape sequence passthrough |

### Category B — Mixed pass/skip (~109 tests)

Files containing a mix of passing tests and tests that require driver improvements. Individual tests that depend on future fixes are marked with `GTEST_SKIP()`.

| File | Total | Pass | Skip | Reason for skip |
|------|-------|------|------|-----------------|
| `test_descriptor.cpp` | ~13 | ~6 | ~7 | Phase 7 (OC-1): SQLCopyDesc crash, SetDescCount allocation |
| `test_connect_options.cpp` | ~36 | ~6 | ~30 | Phase 7/11: CONNECTION_TIMEOUT, ASYNC_ENABLE, QUERY_TIMEOUT, RESET_CONNECTION |
| `test_errors.cpp` | ~18 | ~11 | ~7 | Phase 7 (OC-2/OC-5): DiagRowCount, TruncationIndicator |
| `test_catalogfunctions.cpp` | ~29 | ~26 | ~3 | Phase 11: TypeInfo ordering, GUID searchability, BINARY dedup |
| `test_server_version.cpp` | ~6 | ~4 | ~2 | Phase 4: FB4+ type count in SQLGetTypeInfo |
| `test_scrollable_cursor.cpp` | ~9 | ~5 | ~4 | Phase 4: Scrollable cursor edge cases |

### Category C — All skipped (~167 tests)

These files test features that don't exist on vanilla master. Every test is marked with `GTEST_SKIP()`.

| File | Tests | Reason |
|------|-------|--------|
| `test_null_handles.cpp` | ~65 | Phase 0: NULL handle crash prevention |
| `test_savepoint.cpp` | ~4 | Phase 4: Savepoint isolation |
| `test_conn_settings.cpp` | ~3 | Phase 4: ConnSettings DSN attribute |
| `test_odbc38_compliance.cpp` | ~12 | Phase 8: ODBC 3.8 features |
| `test_guid_and_binary.cpp` | ~14 | Phase 8: SQL_GUID and FB4+ types |
| `test_odbc_string.cpp` | ~26 | Phase 12: OdbcString class (guarded with `__has_include`) |
| `test_phase7_crusher_fixes.cpp` | ~22 | Phase 7: ODBC Crusher bug fixes |
| `test_phase11_typeinfo_timeout_pool.cpp` | ~21 | Phase 11: TypeInfo, timeout, connection pool |

### Excluded

| File | Reason |
|------|--------|
| `bench_fetch.cpp` | Benchmark, not a test — deferred to performance work |

## Expected output

When running against vanilla master, you should see output like:

```
[==========] N tests from M test suites ran.
[  PASSED  ] ~<pass_count> tests.
[  SKIPPED ] ~<skip_count> tests.
[  FAILED  ] 0 tests.
```

**Zero failures** are expected. Tests that would fail on vanilla master are pre-emptively SKIP'd. As driver improvements are merged in future PRs, the corresponding `GTEST_SKIP()` markers will be removed, turning those tests into actual pass/fail tests.

## Connection string format

The `FIREBIRD_ODBC_CONNECTION` environment variable should contain a standard ODBC connection string. Common parameters:

| Parameter | Example | Description |
|-----------|---------|-------------|
| `Driver` | `{Firebird ODBC Driver}` | Registered driver name |
| `Dbname` | `localhost:C:\data\test.fdb` | Server:path to database |
| `Uid` | `SYSDBA` | Username |
| `Pwd` | `masterkey` | Password |
| `Role` | `RDB$ADMIN` | Role (optional) |
| `CharacterSet` | `UTF8` | Character set (optional) |

## Architecture

- Tests link against the **ODBC Driver Manager** (`odbc32`/`odbccp32` on Windows, `libodbc` on Linux) via `find_package(ODBC REQUIRED)`
- They do **not** link against the driver DLL directly (except `test_null_handles.cpp` which uses `LoadLibrary`)
- The driver must be registered as an ODBC driver for the connection string to work
- Google Test is found via `find_package(GTest CONFIG REQUIRED)` — must be pre-installed
