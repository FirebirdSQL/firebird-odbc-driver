// tests/test_cursors.cpp — Scrollable cursor and cursor behavior tests
// (Phase 6, ported from psqlodbc cursors-test)
//
// Tests cursor commit/rollback behavior with large result sets,
// SQL_CURSOR_COMMIT_BEHAVIOR / SQL_CURSOR_ROLLBACK_BEHAVIOR,
// and cursor preservation semantics.

#include "test_helpers.h"
#include <cstring>
#include <string>

class CursorsTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_CURSORS",
            "ID INTEGER NOT NULL PRIMARY KEY, VAL VARCHAR(50)");

        // Insert rows
        for (int i = 1; i <= 100; i++) {
            ReallocStmt();
            char sql[128];
            snprintf(sql, sizeof(sql),
                "INSERT INTO ODBC_TEST_CURSORS (ID, VAL) VALUES (%d, 'foo%d')", i, i);
            ExecDirect(sql);
        }
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        // Restore autocommit
        if (hDbc != SQL_NULL_HDBC) {
            SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
                (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
        }
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;

    // Helper: fetch result set, returning total rows fetched.
    // Optionally commit or rollback after 10 rows.
    // Returns total rows fetched (before and after the transaction action).
    struct FetchResult {
        int rowsFetched;
        bool errorAfterAction;
    };

    FetchResult fetchLargeResult(int actionAfter10) {
        FetchResult result = {0, false};

        SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
            (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
        if (!SQL_SUCCEEDED(ret)) return result;

        ret = SQLExecDirect(hStmt,
            (SQLCHAR*)"SELECT ID, VAL FROM ODBC_TEST_CURSORS ORDER BY ID",
            SQL_NTS);
        if (!SQL_SUCCEEDED(ret)) return result;

        SQLCHAR buf[64];
        SQLLEN ind;
        int i = 0;

        // Fetch first 10 rows
        for (; i < 10; i++) {
            ret = SQLFetch(hStmt);
            if (!SQL_SUCCEEDED(ret)) break;
            SQLGetData(hStmt, 2, SQL_C_CHAR, buf, sizeof(buf), &ind);
        }
        // i == 10 after the loop

        // Perform action
        if (actionAfter10 == 1) {
            // Commit
            ret = SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
        } else if (actionAfter10 == 2) {
            // Rollback
            ret = SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_ROLLBACK);
        }

        // Try to fetch the rest
        for (;; i++) {
            ret = SQLFetch(hStmt);
            if (ret == SQL_NO_DATA) break;
            if (!SQL_SUCCEEDED(ret)) {
                result.errorAfterAction = true;
                break;
            }
            SQLGetData(hStmt, 2, SQL_C_CHAR, buf, sizeof(buf), &ind);
        }

        result.rowsFetched = i;
        SQLFreeStmt(hStmt, SQL_CLOSE);
        return result;
    }
};

// --- Query SQL_CURSOR_COMMIT_BEHAVIOR / SQL_CURSOR_ROLLBACK_BEHAVIOR ---

TEST_F(CursorsTest, QueryCursorCommitBehavior) {
    SQLUSMALLINT info = 0;
    SQLRETURN rc = SQLGetInfo(hDbc, SQL_CURSOR_COMMIT_BEHAVIOR,
        &info, sizeof(info), NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLGetInfo(SQL_CURSOR_COMMIT_BEHAVIOR) failed";
    // Value must be one of SQL_CB_DELETE, SQL_CB_CLOSE, SQL_CB_PRESERVE
    EXPECT_TRUE(info == SQL_CB_DELETE || info == SQL_CB_CLOSE || info == SQL_CB_PRESERVE)
        << "Unexpected commit behavior: " << info;
}

TEST_F(CursorsTest, QueryCursorRollbackBehavior) {
    SQLUSMALLINT info = 0;
    SQLRETURN rc = SQLGetInfo(hDbc, SQL_CURSOR_ROLLBACK_BEHAVIOR,
        &info, sizeof(info), NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLGetInfo(SQL_CURSOR_ROLLBACK_BEHAVIOR) failed";
    EXPECT_TRUE(info == SQL_CB_DELETE || info == SQL_CB_CLOSE || info == SQL_CB_PRESERVE)
        << "Unexpected rollback behavior: " << info;
}

// --- Fetch without interruption ---

TEST_F(CursorsTest, FetchAllWithoutInterruption) {
    auto result = fetchLargeResult(0);  // no commit/rollback
    EXPECT_EQ(result.rowsFetched, 100);
    EXPECT_FALSE(result.errorAfterAction);
}

// --- Fetch with commit mid-stream ---

TEST_F(CursorsTest, FetchWithCommitMidStream) {
    auto result = fetchLargeResult(1);  // commit after 10 rows

    // Depending on SQL_CURSOR_COMMIT_BEHAVIOR, the cursor may close or preserve.
    // Either outcome is valid — but it should not crash.
    if (result.errorAfterAction) {
        // SQL_CB_CLOSE or SQL_CB_DELETE behavior
        EXPECT_EQ(result.rowsFetched, 10)
            << "After commit, expected cursor to be closed at row 10";
    } else {
        // SQL_CB_PRESERVE behavior — all rows should be fetchable
        EXPECT_EQ(result.rowsFetched, 100);
    }
}

// --- Fetch with rollback mid-stream ---

TEST_F(CursorsTest, FetchWithRollbackMidStream) {
    auto result = fetchLargeResult(2);  // rollback after 10 rows

    // Same logic as commit — behavior depends on SQL_CURSOR_ROLLBACK_BEHAVIOR
    if (result.errorAfterAction) {
        EXPECT_EQ(result.rowsFetched, 10);
    } else {
        EXPECT_EQ(result.rowsFetched, 100);
    }
}

// --- Multiple cursors on same connection ---

TEST_F(CursorsTest, MultipleCursorsOnSameConnection) {
    SQLHSTMT hStmt2 = AllocExtraStmt();

    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_CURSORS WHERE ID <= 5 ORDER BY ID",
        SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLExecDirect(hStmt2,
        (SQLCHAR*)"SELECT VAL FROM ODBC_TEST_CURSORS WHERE ID > 95 ORDER BY ID",
        SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Fetch from first
    SQLINTEGER id = 0;
    SQLLEN ind = 0;
    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    SQLGetData(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);
    EXPECT_EQ(id, 1);

    // Fetch from second
    SQLCHAR val[64] = {};
    rc = SQLFetch(hStmt2);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    SQLGetData(hStmt2, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    EXPECT_STREQ((char*)val, "foo96");

    // Interleave more
    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    SQLGetData(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);
    EXPECT_EQ(id, 2);

    SQLFreeStmt(hStmt2, SQL_CLOSE);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
}

// --- Close cursor explicitly, then re-execute ---

TEST_F(CursorsTest, CloseAndReExecute) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_CURSORS ORDER BY ID", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLINTEGER id = 0;
    SQLLEN ind = 0;
    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    SQLGetData(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);
    EXPECT_EQ(id, 1);

    // Close
    rc = SQLCloseCursor(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Re-execute different query
    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_CURSORS ORDER BY ID DESC", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    SQLGetData(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);
    EXPECT_EQ(id, 100);
}

// --- Fetch past end returns SQL_NO_DATA ---

TEST_F(CursorsTest, FetchPastEndReturnsNoData) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFetch(hStmt);
    EXPECT_EQ(rc, SQL_NO_DATA);

    // Fetch again after SQL_NO_DATA should still be SQL_NO_DATA
    rc = SQLFetch(hStmt);
    EXPECT_EQ(rc, SQL_NO_DATA);
}
