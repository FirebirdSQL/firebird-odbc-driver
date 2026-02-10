// test_scrollable_cursor.cpp — Tests for scrollable cursor support (Task 4.7)
#include "test_helpers.h"

// ============================================================================
// ScrollableCursorTest: Validate static scrollable cursor operations
// ============================================================================
class ScrollableCursorTest : public OdbcConnectedTest {
protected:
    static constexpr int NUM_ROWS = 10;

    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (hDbc == SQL_NULL_HDBC) return;

        // Create and populate test table
        ExecIgnoreError("DROP TABLE SCROLL_TEST");
        Commit();
        ReallocStmt();
        ExecDirect("CREATE TABLE SCROLL_TEST (ID INTEGER NOT NULL PRIMARY KEY, NAME VARCHAR(30))");
        Commit();
        ReallocStmt();

        for (int i = 1; i <= NUM_ROWS; ++i) {
            char sql[256];
            snprintf(sql, sizeof(sql),
                "INSERT INTO SCROLL_TEST (ID, NAME) VALUES (%d, 'Row_%02d')", i, i);
            ExecDirect(sql);
            ReallocStmt();
        }
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        if (hDbc != SQL_NULL_HDBC) {
            ExecIgnoreError("DROP TABLE SCROLL_TEST");
            SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
        }
        OdbcConnectedTest::TearDown();
    }

    void OpenScrollableCursor(const char* sql) {
        // Set cursor type to STATIC (scrollable)
        SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_CURSOR_TYPE,
                                        (SQLPOINTER)(intptr_t)SQL_CURSOR_STATIC, 0);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

        ret = SQLSetStmtAttr(hStmt, SQL_ATTR_CURSOR_SCROLLABLE,
                              (SQLPOINTER)(intptr_t)SQL_SCROLLABLE, 0);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));

        ret = SQLExecDirect(hStmt, (SQLCHAR*)sql, SQL_NTS);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    }

    int FetchID() {
        SQLINTEGER id = 0;
        SQLLEN ind = 0;
        SQLGetData(hStmt, 1, SQL_C_SLONG, &id, sizeof(id), &ind);
        return id;
    }
};

TEST_F(ScrollableCursorTest, FetchFirstAndLast) {
    OpenScrollableCursor("SELECT ID, NAME FROM SCROLL_TEST ORDER BY ID");

    // Fetch first
    SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_FIRST, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(FetchID(), 1);

    // Fetch last
    ret = SQLFetchScroll(hStmt, SQL_FETCH_LAST, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(FetchID(), NUM_ROWS);
}

TEST_F(ScrollableCursorTest, FetchPrior) {
    OpenScrollableCursor("SELECT ID, NAME FROM SCROLL_TEST ORDER BY ID");

    // Fetch last
    SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_LAST, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(FetchID(), NUM_ROWS);

    // Fetch prior
    ret = SQLFetchScroll(hStmt, SQL_FETCH_PRIOR, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(FetchID(), NUM_ROWS - 1);
}

TEST_F(ScrollableCursorTest, FetchAbsolute) {
    OpenScrollableCursor("SELECT ID, NAME FROM SCROLL_TEST ORDER BY ID");

    // Fetch absolute row 5
    SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_ABSOLUTE, 5);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(FetchID(), 5);

    // Fetch absolute last row (negative)
    ret = SQLFetchScroll(hStmt, SQL_FETCH_ABSOLUTE, -1);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(FetchID(), NUM_ROWS);
}

TEST_F(ScrollableCursorTest, FetchRelative) {
    OpenScrollableCursor("SELECT ID, NAME FROM SCROLL_TEST ORDER BY ID");

    // Move to row 3
    SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_ABSOLUTE, 3);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(FetchID(), 3);

    // Relative +2 = row 5
    ret = SQLFetchScroll(hStmt, SQL_FETCH_RELATIVE, 2);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(FetchID(), 5);

    // Relative -3 = row 2
    ret = SQLFetchScroll(hStmt, SQL_FETCH_RELATIVE, -3);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(FetchID(), 2);
}

TEST_F(ScrollableCursorTest, FetchNextInScrollable) {
    OpenScrollableCursor("SELECT ID, NAME FROM SCROLL_TEST ORDER BY ID");

    // Fetch next should work in scrollable cursor
    for (int i = 1; i <= NUM_ROWS; ++i) {
        SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_NEXT, 0);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Row " << i << ": " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
        EXPECT_EQ(FetchID(), i);
    }

    // After last row, should get SQL_NO_DATA
    SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_NEXT, 0);
    EXPECT_EQ(ret, SQL_NO_DATA);
}

TEST_F(ScrollableCursorTest, ForwardOnlyRejectsPrior) {
    // With forward-only cursor, SQL_FETCH_PRIOR should fail with HY106
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_CURSOR_TYPE,
                                    (SQLPOINTER)(intptr_t)SQL_CURSOR_FORWARD_ONLY, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT ID FROM SCROLL_TEST ORDER BY ID", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // First fetch should work
    ret = SQLFetchScroll(hStmt, SQL_FETCH_NEXT, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // PRIOR should fail
    ret = SQLFetchScroll(hStmt, SQL_FETCH_PRIOR, 0);
    EXPECT_EQ(ret, SQL_ERROR);
    EXPECT_EQ(GetSqlState(SQL_HANDLE_STMT, hStmt), "HY106");
}

TEST_F(ScrollableCursorTest, FetchBeyondEndReturnsNoData) {
    OpenScrollableCursor("SELECT ID, NAME FROM SCROLL_TEST ORDER BY ID");

    // Fetch absolute beyond end
    SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_ABSOLUTE, NUM_ROWS + 10);
    EXPECT_EQ(ret, SQL_NO_DATA);
}

TEST_F(ScrollableCursorTest, FetchBeforeStartReturnsNoData) {
    OpenScrollableCursor("SELECT ID, NAME FROM SCROLL_TEST ORDER BY ID");

    // Fetch absolute 0 (before first)
    SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_ABSOLUTE, 0);
    EXPECT_EQ(ret, SQL_NO_DATA);
}

TEST_F(ScrollableCursorTest, RewindAfterEnd) {
    OpenScrollableCursor("SELECT ID, NAME FROM SCROLL_TEST ORDER BY ID");

    // Scroll to end
    SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_LAST, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(FetchID(), NUM_ROWS);

    // Try to go past end
    ret = SQLFetchScroll(hStmt, SQL_FETCH_NEXT, 0);
    EXPECT_EQ(ret, SQL_NO_DATA);

    // Rewind to first
    ret = SQLFetchScroll(hStmt, SQL_FETCH_FIRST, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(FetchID(), 1);
}
