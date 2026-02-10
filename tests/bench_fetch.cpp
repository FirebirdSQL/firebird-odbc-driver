// bench_fetch.cpp — Micro-benchmark harness for Firebird ODBC driver (Phase 10)
//
// Benchmarks:
//   (a) Fetch N rows of 10 INT columns
//   (b) Fetch N rows of 5 VARCHAR(100) columns
//   (c) Fetch N rows of 1 BLOB column
//   (d) Batch insert N rows of 10 INT columns
//   (e) W-API overhead: SQLDescribeColW per column
//   (f) Lock overhead: SQLFetch on a 1-row result
//
// Reports: rows/sec, ns/row, ns/col

#include <benchmark/benchmark.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string GetConnectionString() {
    const char* cs = std::getenv("FIREBIRD_ODBC_CONNECTION");
    return cs ? std::string(cs) : std::string();
}

struct OdbcHandles {
    SQLHENV  hEnv  = SQL_NULL_HENV;
    SQLHDBC  hDbc  = SQL_NULL_HDBC;
    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    bool     ok    = false;

    OdbcHandles() {
        auto cs = GetConnectionString();
        if (cs.empty()) return;

        if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv))) return;
        SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc))) return;

        SQLCHAR out[1024]; SQLSMALLINT outLen;
        if (!SQL_SUCCEEDED(SQLDriverConnect(hDbc, NULL, (SQLCHAR*)cs.c_str(), SQL_NTS,
                                            out, sizeof(out), &outLen, SQL_DRIVER_NOPROMPT)))
            return;

        if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt))) return;
        ok = true;
    }

    ~OdbcHandles() {
        if (hStmt != SQL_NULL_HSTMT) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        if (hDbc  != SQL_NULL_HDBC)  { SQLDisconnect(hDbc); SQLFreeHandle(SQL_HANDLE_DBC, hDbc); }
        if (hEnv  != SQL_NULL_HENV)  SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    }

    SQLHSTMT freshStmt() {
        if (hStmt != SQL_NULL_HSTMT) {
            SQLFreeStmt(hStmt, SQL_CLOSE);
            SQLFreeStmt(hStmt, SQL_UNBIND);
            SQLFreeStmt(hStmt, SQL_RESET_PARAMS);
        }
        return hStmt;
    }

    void execDirect(const char* sql) {
        SQLExecDirect(hStmt, (SQLCHAR*)sql, SQL_NTS);
    }

    void execIgnoreError(const char* sql) {
        SQLHSTMT tmp = SQL_NULL_HSTMT;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &tmp);
        SQLExecDirect(tmp, (SQLCHAR*)sql, SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, tmp);
    }

    void commit() { SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT); }
};

// ---------------------------------------------------------------------------
// Table setup helpers
// ---------------------------------------------------------------------------

static const int kDefaultRowCount = 10000;

static void EnsureIntTable(OdbcHandles& h, int nRows) {
    h.execIgnoreError("DROP TABLE BENCH_INT10");
    h.commit();

    // 10 INT columns
    h.freshStmt();
    h.execDirect("CREATE TABLE BENCH_INT10 ("
                 "C1 INTEGER, C2 INTEGER, C3 INTEGER, C4 INTEGER, C5 INTEGER, "
                 "C6 INTEGER, C7 INTEGER, C8 INTEGER, C9 INTEGER, C10 INTEGER)");
    h.commit();

    // Populate using EXECUTE BLOCK for speed
    h.freshStmt();
    std::ostringstream oss;
    oss << "EXECUTE BLOCK AS\n"
        << "  DECLARE VARIABLE I INTEGER = 0;\n"
        << "BEGIN\n"
        << "  WHILE (I < " << nRows << ") DO\n"
        << "  BEGIN\n"
        << "    INSERT INTO BENCH_INT10 VALUES (I, I+1, I+2, I+3, I+4, I+5, I+6, I+7, I+8, I+9);\n"
        << "    I = I + 1;\n"
        << "  END\n"
        << "END";
    h.execDirect(oss.str().c_str());
    h.commit();
}

static void EnsureVarcharTable(OdbcHandles& h, int nRows) {
    h.execIgnoreError("DROP TABLE BENCH_VC5");
    h.commit();

    h.freshStmt();
    h.execDirect("CREATE TABLE BENCH_VC5 ("
                 "C1 VARCHAR(100), C2 VARCHAR(100), C3 VARCHAR(100), "
                 "C4 VARCHAR(100), C5 VARCHAR(100))");
    h.commit();

    h.freshStmt();
    std::ostringstream oss;
    oss << "EXECUTE BLOCK AS\n"
        << "  DECLARE VARIABLE I INTEGER = 0;\n"
        << "  DECLARE VARIABLE S VARCHAR(100);\n"
        << "BEGIN\n"
        << "  WHILE (I < " << nRows << ") DO\n"
        << "  BEGIN\n"
        << "    S = 'Row_' || CAST(I AS VARCHAR(20)) || '_data_padding_to_make_string_longer_xxxxxxxxxxxxxx';\n"
        << "    INSERT INTO BENCH_VC5 VALUES (S, S, S, S, S);\n"
        << "    I = I + 1;\n"
        << "  END\n"
        << "END";
    h.execDirect(oss.str().c_str());
    h.commit();
}

static void EnsureBlobTable(OdbcHandles& h, int nRows) {
    h.execIgnoreError("DROP TABLE BENCH_BLOB1");
    h.commit();

    h.freshStmt();
    h.execDirect("CREATE TABLE BENCH_BLOB1 (C1 BLOB SUB_TYPE TEXT)");
    h.commit();

    // Insert rows with small text blobs (~200 bytes each)
    h.freshStmt();
    std::ostringstream oss;
    oss << "EXECUTE BLOCK AS\n"
        << "  DECLARE VARIABLE I INTEGER = 0;\n"
        << "BEGIN\n"
        << "  WHILE (I < " << nRows << ") DO\n"
        << "  BEGIN\n"
        << "    INSERT INTO BENCH_BLOB1 VALUES ("
        << "'Blob data row ' || CAST(I AS VARCHAR(20)) || "
        << "' - padding to make this a reasonable size blob for benchmarking purposes. "
        << "This is approximately two hundred bytes of text data which simulates a typical text blob scenario.');\n"
        << "    I = I + 1;\n"
        << "  END\n"
        << "END";
    h.execDirect(oss.str().c_str());
    h.commit();
}

// ---------------------------------------------------------------------------
// (a) Fetch 10 INT columns
// ---------------------------------------------------------------------------

static void BM_FetchInt10(benchmark::State& state) {
    const int nRows = static_cast<int>(state.range(0));
    OdbcHandles h;
    if (!h.ok) { state.SkipWithError("No ODBC connection"); return; }

    EnsureIntTable(h, nRows);

    SQLINTEGER cols[10];
    SQLLEN     inds[10];

    for (auto _ : state) {
        h.freshStmt();
        SQLExecDirect(h.hStmt, (SQLCHAR*)"SELECT * FROM BENCH_INT10", SQL_NTS);

        for (int i = 0; i < 10; ++i)
            SQLBindCol(h.hStmt, (SQLUSMALLINT)(i + 1), SQL_C_SLONG, &cols[i], 0, &inds[i]);

        int rowsFetched = 0;
        while (SQLFetch(h.hStmt) == SQL_SUCCESS)
            ++rowsFetched;

        benchmark::DoNotOptimize(rowsFetched);
        benchmark::DoNotOptimize(cols);
    }

    state.SetItemsProcessed(state.iterations() * nRows);
    state.counters["rows/s"] = benchmark::Counter(
        static_cast<double>(nRows), benchmark::Counter::kIsIterationInvariantRate);
    state.counters["ns/row"] = benchmark::Counter(
        static_cast<double>(nRows), benchmark::Counter::kIsIterationInvariantRate |
        benchmark::Counter::kInvert);

    h.execIgnoreError("DROP TABLE BENCH_INT10");
    h.commit();
}
BENCHMARK(BM_FetchInt10)->Arg(kDefaultRowCount)->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// (b) Fetch 5 VARCHAR(100) columns
// ---------------------------------------------------------------------------

static void BM_FetchVarchar5(benchmark::State& state) {
    const int nRows = static_cast<int>(state.range(0));
    OdbcHandles h;
    if (!h.ok) { state.SkipWithError("No ODBC connection"); return; }

    EnsureVarcharTable(h, nRows);

    char cols[5][104];
    SQLLEN inds[5];

    for (auto _ : state) {
        h.freshStmt();
        SQLExecDirect(h.hStmt, (SQLCHAR*)"SELECT * FROM BENCH_VC5", SQL_NTS);

        for (int i = 0; i < 5; ++i)
            SQLBindCol(h.hStmt, (SQLUSMALLINT)(i + 1), SQL_C_CHAR, cols[i], sizeof(cols[i]), &inds[i]);

        int rowsFetched = 0;
        while (SQLFetch(h.hStmt) == SQL_SUCCESS)
            ++rowsFetched;

        benchmark::DoNotOptimize(rowsFetched);
        benchmark::DoNotOptimize(cols);
    }

    state.SetItemsProcessed(state.iterations() * nRows);
    state.counters["rows/s"] = benchmark::Counter(
        static_cast<double>(nRows), benchmark::Counter::kIsIterationInvariantRate);
    state.counters["ns/row"] = benchmark::Counter(
        static_cast<double>(nRows), benchmark::Counter::kIsIterationInvariantRate |
        benchmark::Counter::kInvert);

    h.execIgnoreError("DROP TABLE BENCH_VC5");
    h.commit();
}
BENCHMARK(BM_FetchVarchar5)->Arg(kDefaultRowCount)->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// (c) Fetch 1 BLOB column (text sub_type)
// ---------------------------------------------------------------------------

static void BM_FetchBlob1(benchmark::State& state) {
    const int nRows = static_cast<int>(state.range(0));
    OdbcHandles h;
    if (!h.ok) { state.SkipWithError("No ODBC connection"); return; }

    EnsureBlobTable(h, nRows);

    char buf[1024];
    SQLLEN ind;

    for (auto _ : state) {
        h.freshStmt();
        SQLExecDirect(h.hStmt, (SQLCHAR*)"SELECT * FROM BENCH_BLOB1", SQL_NTS);

        SQLBindCol(h.hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);

        int rowsFetched = 0;
        while (SQLFetch(h.hStmt) == SQL_SUCCESS)
            ++rowsFetched;

        benchmark::DoNotOptimize(rowsFetched);
        benchmark::DoNotOptimize(buf);
    }

    state.SetItemsProcessed(state.iterations() * nRows);
    state.counters["rows/s"] = benchmark::Counter(
        static_cast<double>(nRows), benchmark::Counter::kIsIterationInvariantRate);
    state.counters["ns/row"] = benchmark::Counter(
        static_cast<double>(nRows), benchmark::Counter::kIsIterationInvariantRate |
        benchmark::Counter::kInvert);

    h.execIgnoreError("DROP TABLE BENCH_BLOB1");
    h.commit();
}
BENCHMARK(BM_FetchBlob1)->Arg(1000)->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// (d) Batch insert 10 INT columns
// ---------------------------------------------------------------------------

static void BM_InsertInt10(benchmark::State& state) {
    const int nRows = static_cast<int>(state.range(0));
    OdbcHandles h;
    if (!h.ok) { state.SkipWithError("No ODBC connection"); return; }

    for (auto _ : state) {
        state.PauseTiming();
        h.execIgnoreError("DROP TABLE BENCH_INS_INT10");
        h.commit();
        h.freshStmt();
        h.execDirect("CREATE TABLE BENCH_INS_INT10 ("
                     "C1 INTEGER, C2 INTEGER, C3 INTEGER, C4 INTEGER, C5 INTEGER, "
                     "C6 INTEGER, C7 INTEGER, C8 INTEGER, C9 INTEGER, C10 INTEGER)");
        h.commit();
        h.freshStmt();

        SQLPrepare(h.hStmt, (SQLCHAR*)
            "INSERT INTO BENCH_INS_INT10 VALUES (?,?,?,?,?,?,?,?,?,?)", SQL_NTS);

        SQLINTEGER params[10];
        SQLLEN     paramInds[10];
        for (int i = 0; i < 10; ++i) {
            paramInds[i] = 0;
            SQLBindParameter(h.hStmt, (SQLUSMALLINT)(i + 1), SQL_PARAM_INPUT,
                             SQL_C_SLONG, SQL_INTEGER, 0, 0, &params[i], 0, &paramInds[i]);
        }
        state.ResumeTiming();

        for (int row = 0; row < nRows; ++row) {
            for (int i = 0; i < 10; ++i)
                params[i] = row * 10 + i;
            SQLExecute(h.hStmt);
        }

        state.PauseTiming();
        h.commit();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * nRows);
    state.counters["rows/s"] = benchmark::Counter(
        static_cast<double>(nRows), benchmark::Counter::kIsIterationInvariantRate);
    state.counters["ns/row"] = benchmark::Counter(
        static_cast<double>(nRows), benchmark::Counter::kIsIterationInvariantRate |
        benchmark::Counter::kInvert);

    h.execIgnoreError("DROP TABLE BENCH_INS_INT10");
    h.commit();
}
BENCHMARK(BM_InsertInt10)->Arg(kDefaultRowCount)->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// (e) W-API overhead: SQLDescribeColW
// ---------------------------------------------------------------------------

#ifdef _WIN32
static void BM_DescribeColW(benchmark::State& state) {
    OdbcHandles h;
    if (!h.ok) { state.SkipWithError("No ODBC connection"); return; }

    EnsureIntTable(h, 1);

    for (auto _ : state) {
        h.freshStmt();
        SQLExecDirect(h.hStmt, (SQLCHAR*)"SELECT * FROM BENCH_INT10", SQL_NTS);

        SQLWCHAR colName[128];
        SQLSMALLINT nameLen, dataType, decDigits, nullable;
        SQLULEN colSize;

        for (int i = 1; i <= 10; ++i) {
            SQLDescribeColW(h.hStmt, (SQLUSMALLINT)i, colName, 128,
                            &nameLen, &dataType, &colSize, &decDigits, &nullable);
        }
        benchmark::DoNotOptimize(colName);
    }

    state.counters["calls/iter"] = 10;

    h.execIgnoreError("DROP TABLE BENCH_INT10");
    h.commit();
}
BENCHMARK(BM_DescribeColW)->Unit(benchmark::kMicrosecond);
#endif

// ---------------------------------------------------------------------------
// (f) Lock overhead: SQLFetch on tiny result
// ---------------------------------------------------------------------------

static void BM_FetchSingleRow(benchmark::State& state) {
    OdbcHandles h;
    if (!h.ok) { state.SkipWithError("No ODBC connection"); return; }

    // "SELECT 1 FROM RDB$DATABASE" returns exactly 1 row
    SQLINTEGER val;
    SQLLEN ind;

    for (auto _ : state) {
        h.freshStmt();
        SQLExecDirect(h.hStmt, (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
        SQLBindCol(h.hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
        SQLFetch(h.hStmt);
        benchmark::DoNotOptimize(val);
    }
}
BENCHMARK(BM_FetchSingleRow)->Unit(benchmark::kMicrosecond);

// ---------------------------------------------------------------------------

BENCHMARK_MAIN();
