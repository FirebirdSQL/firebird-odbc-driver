# AI Agent Instructions: Firebird ODBC Driver

## RULE #0: Complete Work Verification

Build, test, commit, push, and monitor CI workflow after each task. Fix failures and repeat until successful.

Use the `gh` command with non-interactive switches to check the CI workflows and logs.

## Test Database Connection

Firebird 5.0 database available in dev and CI. Use `FIREBIRD_ODBC_CONNECTION` environment variable:
```
Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=/fbodbc-tests/fb502/fbclient.dll
```

## RULE #1: Update Master Plan

Update [Docs\FIREBIRD_ODBC_MASTER_PLAN.md](Docs\FIREBIRD_ODBC_MASTER_PLAN.md) for: phases/milestones, new tests/dependencies/features, structure/architecture changes. Skip for: typos, minor bugs, comments, patch updates.

## RULE #2: Use ./tmp for Temporary Files  

All temporary files (test outputs, scripts, investigation data) go in `./tmp/`.

## Code Standards

**C++17 minimum**

**Naming:**
- Classes: `PascalCase`
- Functions/Methods: `snake_case`
- Members: `snake_case_` (trailing underscore)
- Constants: `kPascalCase` or `UPPER_CASE`
- Namespaces: `snake_case`

**Headers:** Use `#pragma once`. Include order: corresponding header, C++ std, third-party, project headers.

**Testing:** Google Test in `tests/`. Files: `test_*.cpp`. Fixtures: `*Test`. Use `TEST_F()` or `TEST()`.

**CMake:** 3.20+. Use modern target-based CMake and `FetchContent` for dependencies.

**Commits:** Follow conventional commits format (feat:, fix:, docs:, test:, refactor:, perf:, build:, ci:).

## Build & Test

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Build types: Debug, Release, RelWithDebInfo, MinSizeRel.

## ODBC References

- [ODBC API Reference](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/odbc-api-reference)
- [ODBC Programmer's Reference](https://learn.microsoft.com/en-us/sql/odbc/reference/odbc-programmer-s-reference)
- [unixODBC](http://www.unixodbc.org/)

Key concepts: Handle hierarchy (Environment → Connection → Statement/Descriptor), return codes (SQL_SUCCESS, SQL_ERROR, etc.), diagnostic records via `SQLGetDiagRec`.

## Quality Checklist

- [ ] Compiles without warnings (Windows & Linux)
- [ ] All tests pass
- [ ] Follows naming conventions
- [ ] Public APIs documented
- [ ] RAII for all handles
- [ ] Full diagnostic error handling
- [ ] No memory leaks
- [ ] Master plan updated (if applicable)
- [ ] Conventional commit message

## Best Practices

- Use `std::string_view` for read-only strings
- Use `std::optional` for expected failures
- Pre-allocate ODBC fetch buffers
- Use move semantics
- Extract repetitive code to helpers
- Abstract platform-specific code
- Aim for <1ms test overhead

*Last Updated: February 5, 2026*
