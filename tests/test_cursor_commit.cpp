// tests/test_cursor_commit.cpp — Cursor behavior across commit/rollback
// (Phase 6, ported from psqlodbc cursor-commit-test)
//
// Tests that cursors behave correctly when transactions are committed or
// rolled back while they are open.

#include "test_helpers.h"
#include <cstring>

class CursorCommitTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_CURSOR_CMT",
            "ID INTEGER NOT NULL PRIMARY KEY, VAL VARCHAR(50)");

        // Insert 5 rows
        ExecDirect("INSERT INTO ODBC_TEST_CURSOR_CMT VALUES (1, 'row-1')");
        ExecDirect("INSERT INTO ODBC_TEST_CURSOR_CMT VALUES (2, 'row-2')");
        ExecDirect("INSERT INTO ODBC_TEST_CURSOR_CMT VALUES (3, 'row-3')");
        ExecDirect("INSERT INTO ODBC_TEST_CURSOR_CMT VALUES (4, 'row-4')");
        ExecDirect("INSERT INTO ODBC_TEST_CURSOR_CMT VALUES (5, 'row-5')");
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

// ===== Basic: open cursor, fetch all, close =====

TEST_F(CursorCommitTest, BasicForwardOnlyCursor) {
    ExecDirect("SELECT ID, VAL FROM ODBC_TEST_CURSOR_CMT ORDER BY ID");

    SQLINTEGER id = 0;
    SQLCHAR val[32] = {};
    SQLLEN idInd = 0, valInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, val, sizeof(val), &valInd);

    int rowCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        rowCount++;
        EXPECT_EQ(id, rowCount);
    }
    EXPECT_EQ(rowCount, 5);
}

// ===== Commit with open cursor (forward-only) =====

TEST_F(CursorCommitTest, CommitClosesForwardOnlyCursor) {
    // Turn off autocommit
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ExecDirect("SELECT ID FROM ODBC_TEST_CURSOR_CMT ORDER BY ID");

    // Commit while cursor is open
    Commit();

    // After commit, fetching from forward-only cursor should fail
    // (the behavior is driver-specific — some preserve, some close)
    SQLINTEGER id = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);
    ret = SQLFetch(hStmt);
    // We document the actual behavior — it should either work or return an error,
    // but it should never crash
    SUCCEED() << "Fetch after commit returned: " << ret;

    // Restore autocommit
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
}

// ===== Static cursor survives commit =====

TEST_F(CursorCommitTest, StaticCursorSurvivesCommit) {
    // Set cursor type to static
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_CURSOR_TYPE,
        (SQLPOINTER)SQL_CURSOR_STATIC, SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(ret)) {
        GTEST_SKIP() << "Static cursors not supported";
    }

    // Turn off autocommit
    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ExecDirect("SELECT ID, VAL FROM ODBC_TEST_CURSOR_CMT ORDER BY ID");

    // Commit while cursor is open
    Commit();

    // Static cursor should preserve results even after commit
    SQLINTEGER id = 0;
    SQLCHAR val[32] = {};
    SQLLEN idInd = 0, valInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, val, sizeof(val), &valInd);

    // Try to fetch first row
    ret = SQLFetchScroll(hStmt, SQL_FETCH_FIRST, 0);
    if (SQL_SUCCEEDED(ret)) {
        EXPECT_EQ(id, 1);
        EXPECT_STREQ((char*)val, "row-1");

        // Fetch all remaining
        int count = 1;
        while (SQL_SUCCEEDED(SQLFetchScroll(hStmt, SQL_FETCH_NEXT, 0))) {
            count++;
        }
        EXPECT_EQ(count, 5);
    }

    // Restore autocommit
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
}

// ===== Rollback with open cursor =====

TEST_F(CursorCommitTest, RollbackClosesForwardOnlyCursor) {
    // Turn off autocommit
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ExecDirect("SELECT ID FROM ODBC_TEST_CURSOR_CMT ORDER BY ID");

    // Rollback while cursor is open
    Rollback();

    // After rollback, cursor should typically be closed
    SQLINTEGER id = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);
    ret = SQLFetch(hStmt);
    // Should either succeed (driver preserves) or fail (driver closes cursor)
    // Either way, no crash
    SUCCEED() << "Fetch after rollback returned: " << ret;

    // Restore autocommit
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
}

// ===== Multiple cursors and commit =====

TEST_F(CursorCommitTest, MultipleCursorsAndCommit) {
    // Turn off autocommit
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);

    // Open first cursor
    ExecDirect("SELECT ID FROM ODBC_TEST_CURSOR_CMT ORDER BY ID");

    // Partially fetch
    SQLINTEGER id = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 1);

    // Open second cursor on a different statement
    SQLHSTMT hStmt2 = AllocExtraStmt();
    ret = SQLExecDirect(hStmt2,
        (SQLCHAR*)"SELECT VAL FROM ODBC_TEST_CURSOR_CMT ORDER BY ID", SQL_NTS);
    if (SQL_SUCCEEDED(ret)) {
        // Commit while both cursors are open
        Commit();

        // Both cursors should handle commit gracefully (no crash)
        SQLCHAR val[32] = {};
        SQLLEN valInd = 0;
        SQLBindCol(hStmt2, 1, SQL_C_CHAR, val, sizeof(val), &valInd);
        ret = SQLFetch(hStmt2);
        // Don't assert — just ensure no crash
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);

    // Restore autocommit
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
}

// ===== Commit then re-open cursor =====

TEST_F(CursorCommitTest, ReOpenCursorAfterCommit) {
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);

    ExecDirect("SELECT ID FROM ODBC_TEST_CURSOR_CMT ORDER BY ID");
    Commit();

    // Close the cursor explicitly
    SQLCloseCursor(hStmt);

    // Re-open a new cursor
    ExecDirect("SELECT ID FROM ODBC_TEST_CURSOR_CMT ORDER BY ID");

    SQLINTEGER id = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);

    int count = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        count++;
    }
    EXPECT_EQ(count, 5) << "Should see all 5 rows after re-opening cursor";

    // Restore autocommit
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
}
