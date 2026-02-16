// tests/test_stmthandles.cpp — Statement handle stress tests
// (Phase 6, ported from psqlodbc stmthandles-test)
//
// Tests that many simultaneous statement handles work correctly,
// including allocation, interleaved execution, and prepare/execute
// with SQLNumResultCols before execute.

#include "test_helpers.h"
#include <cstring>
#include <string>
#include <vector>

class StmtHandlesTest : public OdbcConnectedTest {};

// --- Allocate NUM_HANDLES statement handles and execute a query on each ---

TEST_F(StmtHandlesTest, AllocateAndExecuteMany) {
    constexpr int NUM_HANDLES = 100;
    std::vector<SQLHSTMT> handles(NUM_HANDLES, SQL_NULL_HSTMT);
    int nAllocated = 0;

    // Allocate many statement handles
    for (int i = 0; i < NUM_HANDLES; i++) {
        SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &handles[i]);
        if (!SQL_SUCCEEDED(rc)) {
            // Some drivers may have limits; stop here
            break;
        }
        nAllocated++;
    }
    ASSERT_GE(nAllocated, 50)
        << "Could not allocate at least 50 statement handles";

    // Execute a query on each
    for (int i = 0; i < nAllocated; i++) {
        char sql[64];
        snprintf(sql, sizeof(sql), "SELECT 'stmt no %d' FROM RDB$DATABASE", i + 1);
        SQLRETURN rc = SQLExecDirect(handles[i], (SQLCHAR*)sql, SQL_NTS);
        ASSERT_TRUE(SQL_SUCCEEDED(rc))
            << "ExecDirect failed on handle #" << (i + 1)
            << ": " << GetOdbcError(SQL_HANDLE_STMT, handles[i]);
    }

    // Verify results from a sample of them
    for (int i = 0; i < nAllocated; i += (nAllocated / 10)) {
        SQLCHAR buf[64] = {};
        SQLLEN ind = 0;
        SQLRETURN rc = SQLFetch(handles[i]);
        ASSERT_TRUE(SQL_SUCCEEDED(rc))
            << "Fetch failed on handle #" << (i + 1);
        rc = SQLGetData(handles[i], 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));

        char expected[64];
        snprintf(expected, sizeof(expected), "stmt no %d", i + 1);
        EXPECT_STREQ((char*)buf, expected);
    }

    // Close and free all handles
    for (int i = 0; i < nAllocated; i++) {
        SQLFreeStmt(handles[i], SQL_CLOSE);
        SQLFreeHandle(SQL_HANDLE_STMT, handles[i]);
    }
}

// --- Interleaved prepare/execute on multiple statements ---

TEST_F(StmtHandlesTest, InterleavedPrepareExecute) {
    constexpr int NUM_STMTS = 5;
    SQLHSTMT stmts[NUM_STMTS] = {};

    for (int i = 0; i < NUM_STMTS; i++) {
        SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &stmts[i]);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
    }

    // Prepare all statements first (interleaved) — each has a different
    // number of result columns
    for (int i = 0; i < NUM_STMTS; i++) {
        std::string sql = "SELECT 'stmt'";
        for (int j = 0; j < i; j++) {
            sql += ", 'col" + std::to_string(j) + "'";
        }
        sql += " FROM RDB$DATABASE";
        SQLRETURN rc = SQLPrepare(stmts[i], (SQLCHAR*)sql.c_str(), SQL_NTS);
        ASSERT_TRUE(SQL_SUCCEEDED(rc))
            << "Prepare #" << i << " failed: "
            << GetOdbcError(SQL_HANDLE_STMT, stmts[i]);
    }

    // Test SQLNumResultCols BEFORE SQLExecute (ODBC spec says this is valid
    // after SQLPrepare)
    for (int i = 0; i < NUM_STMTS; i++) {
        SQLSMALLINT colcount = 0;
        SQLRETURN rc = SQLNumResultCols(stmts[i], &colcount);
        ASSERT_TRUE(SQL_SUCCEEDED(rc))
            << "NumResultCols #" << i << " failed: "
            << GetOdbcError(SQL_HANDLE_STMT, stmts[i]);
        EXPECT_EQ(colcount, i + 1) << "Wrong column count for stmt #" << i;
    }

    // Execute all statements
    for (int i = 0; i < NUM_STMTS; i++) {
        SQLRETURN rc = SQLExecute(stmts[i]);
        ASSERT_TRUE(SQL_SUCCEEDED(rc))
            << "Execute #" << i << " failed: "
            << GetOdbcError(SQL_HANDLE_STMT, stmts[i]);
    }

    // Fetch results from each
    for (int i = 0; i < NUM_STMTS; i++) {
        SQLRETURN rc = SQLFetch(stmts[i]);
        ASSERT_TRUE(SQL_SUCCEEDED(rc))
            << "Fetch #" << i << " failed";

        // Verify first column always says "stmt"
        SQLCHAR buf[64] = {};
        SQLLEN ind = 0;
        rc = SQLGetData(stmts[i], 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
        EXPECT_STREQ((char*)buf, "stmt");

        // No more rows
        rc = SQLFetch(stmts[i]);
        EXPECT_EQ(rc, SQL_NO_DATA);
    }

    // Cleanup
    for (int i = 0; i < NUM_STMTS; i++) {
        SQLFreeStmt(stmts[i], SQL_CLOSE);
        SQLFreeHandle(SQL_HANDLE_STMT, stmts[i]);
    }
}

// --- Allocate, free some in the middle, then reuse ---

TEST_F(StmtHandlesTest, AllocFreeReallocPattern) {
    constexpr int NUM = 20;
    SQLHSTMT stmts[NUM] = {};

    // Allocate all
    for (int i = 0; i < NUM; i++) {
        SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &stmts[i]);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
    }

    // Free even-numbered handles
    for (int i = 0; i < NUM; i += 2) {
        SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_STMT, stmts[i]);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
        stmts[i] = SQL_NULL_HSTMT;
    }

    // Reallocate the freed slots
    for (int i = 0; i < NUM; i += 2) {
        SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &stmts[i]);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
    }

    // Execute on all and verify
    for (int i = 0; i < NUM; i++) {
        char sql[64];
        snprintf(sql, sizeof(sql), "SELECT %d FROM RDB$DATABASE", i);
        SQLRETURN rc = SQLExecDirect(stmts[i], (SQLCHAR*)sql, SQL_NTS);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));

        SQLINTEGER val = -1;
        SQLLEN ind = 0;
        SQLBindCol(stmts[i], 1, SQL_C_SLONG, &val, 0, &ind);
        rc = SQLFetch(stmts[i]);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
        EXPECT_EQ(val, i);
    }

    // Cleanup
    for (int i = 0; i < NUM; i++) {
        if (stmts[i] != SQL_NULL_HSTMT) {
            SQLFreeStmt(stmts[i], SQL_CLOSE);
            SQLFreeHandle(SQL_HANDLE_STMT, stmts[i]);
        }
    }
}

// --- Reuse same handle: exec, close, exec, close ---

TEST_F(StmtHandlesTest, ReuseAfterClose) {
    for (int iter = 0; iter < 10; iter++) {
        char sql[64];
        snprintf(sql, sizeof(sql), "SELECT %d FROM RDB$DATABASE", iter);
        SQLRETURN rc = SQLExecDirect(hStmt, (SQLCHAR*)sql, SQL_NTS);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));

        SQLINTEGER val = -1;
        SQLLEN ind = 0;
        rc = SQLGetData(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
        // Need to fetch first
        rc = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
        rc = SQLGetData(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
        EXPECT_EQ(val, iter);

        rc = SQLFreeStmt(hStmt, SQL_CLOSE);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
    }
}
