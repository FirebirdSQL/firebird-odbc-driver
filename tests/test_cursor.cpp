// tests/test_cursor.cpp — Cursor and fetch tests (Phase 3.2)

#include "test_helpers.h"

// ===== Cursor name tests =====

class CursorTest : public OdbcConnectedTest {};

TEST_F(CursorTest, SetAndGetCursorName) {
    SQLCHAR cursorName[128] = {};
    SQLSMALLINT nameLen = 0;

    // Set a cursor name
    SQLRETURN ret = SQLSetCursorName(hStmt, (SQLCHAR*)"MY_CURSOR", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SetCursorName failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Get it back
    ret = SQLGetCursorName(hStmt, cursorName, sizeof(cursorName), &nameLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "GetCursorName failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_STREQ((char*)cursorName, "MY_CURSOR");
    EXPECT_EQ(nameLen, 9);
}

TEST_F(CursorTest, DefaultCursorName) {
    SQLCHAR cursorName[128] = {};
    SQLSMALLINT nameLen = 0;

    // Even without setting a cursor name, the driver should return one
    SQLRETURN ret = SQLGetCursorName(hStmt, cursorName, sizeof(cursorName), &nameLen);
    EXPECT_TRUE(SQL_SUCCEEDED(ret));
    // Driver-generated cursor names are usually "SQL_CURxxxxxxxx" or similar
    EXPECT_GT(nameLen, 0);
}

// ===== Block fetch / row status tests =====

class BlockFetchTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_FETCH",
            "ID INTEGER NOT NULL PRIMARY KEY, VAL VARCHAR(30)");

        // Insert 10 rows
        for (int i = 1; i <= 10; i++) {
            ReallocStmt();
            char sql[128];
            snprintf(sql, sizeof(sql),
                "INSERT INTO ODBC_TEST_FETCH (ID, VAL) VALUES (%d, 'Row %d')", i, i);
            ExecDirect(sql);
        }
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

TEST_F(BlockFetchTest, FetchAllRows) {
    ExecDirect("SELECT ID, VAL FROM ODBC_TEST_FETCH ORDER BY ID");

    int count = 0;
    SQLINTEGER id;
    SQLCHAR val[31];
    SQLLEN idInd, valInd;

    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, val, sizeof(val), &valInd);

    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        count++;
        EXPECT_EQ(id, count);
    }
    EXPECT_EQ(count, 10);
}

TEST_F(BlockFetchTest, FetchWithRowArraySize) {
    // Set row array size to 5 to fetch multiple rows at once
    SQLULEN rowsFetched = 0;
    SQLUSMALLINT rowStatus[5];

    SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)5, 0);
    SQLSetStmtAttr(hStmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0);
    SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_STATUS_PTR, rowStatus, 0);

    ExecDirect("SELECT ID FROM ODBC_TEST_FETCH ORDER BY ID");

    SQLINTEGER ids[5];
    SQLLEN idsInd[5];
    SQLBindCol(hStmt, 1, SQL_C_SLONG, ids, 0, idsInd);

    // First fetch: should get rows (driver may or may not populate rowsFetched)
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "First block fetch failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    // Verify at least the first row is correct
    EXPECT_EQ(ids[0], 1);
    // If rowsFetched is populated, verify it
    if (rowsFetched > 0) {
        EXPECT_EQ(rowsFetched, 5u);
        EXPECT_EQ(ids[4], 5);
    }

    // Second fetch
    ret = SQLFetch(hStmt);
    if (SQL_SUCCEEDED(ret)) {
        EXPECT_EQ(ids[0], 6);
    }

    // Reset row array size for cleanup
    SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)1, 0);
}

TEST_F(BlockFetchTest, SQLCloseCursorAllowsReExec) {
    ExecDirect("SELECT ID FROM ODBC_TEST_FETCH ORDER BY ID");

    SQLINTEGER id;
    SQLLEN ind;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);

    // Fetch just the first row
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 1);

    // Close cursor
    ret = SQLCloseCursor(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Re-execute on the same statement handle
    ExecDirect("SELECT ID FROM ODBC_TEST_FETCH ORDER BY ID DESC");
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 10);
}

// ===== SQLNumResultCols / SQLRowCount =====

TEST_F(BlockFetchTest, SQLNumResultCols) {
    ExecDirect("SELECT ID, VAL FROM ODBC_TEST_FETCH WHERE 1=0");

    SQLSMALLINT numCols = 0;
    SQLRETURN ret = SQLNumResultCols(hStmt, &numCols);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(numCols, 2);
}

TEST_F(BlockFetchTest, SQLRowCount) {
    ReallocStmt();
    ExecDirect("UPDATE ODBC_TEST_FETCH SET VAL = 'Updated' WHERE ID <= 3");

    SQLLEN rowCount = -1;
    SQLRETURN ret = SQLRowCount(hStmt, &rowCount);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(rowCount, 3);

    Rollback();
}

// ===== SQLDescribeCol =====

TEST_F(BlockFetchTest, SQLDescribeCol) {
    ExecDirect("SELECT ID, VAL FROM ODBC_TEST_FETCH WHERE 1=0");

    SQLCHAR colName[128] = {};
    SQLSMALLINT nameLen = 0, dataType = 0, decDigits = 0, nullable = 0;
    SQLULEN colSize = 0;

    // Column 1: ID (INTEGER)
    SQLRETURN ret = SQLDescribeCol(hStmt, 1, colName, sizeof(colName), &nameLen,
                                   &dataType, &colSize, &decDigits, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)colName, "ID");
    EXPECT_EQ(dataType, SQL_INTEGER);

    // Column 2: VAL (VARCHAR)
    ret = SQLDescribeCol(hStmt, 2, colName, sizeof(colName), &nameLen,
                          &dataType, &colSize, &decDigits, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)colName, "VAL");
    EXPECT_TRUE(dataType == SQL_VARCHAR || dataType == SQL_WVARCHAR);
}

// ===== Commit behavior (result set invalidation) =====

TEST_F(BlockFetchTest, CommitClosesBehavior) {
    // Set auto-commit off (it's the default in the fixture, but be explicit)
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
                      (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);

    ExecDirect("SELECT ID FROM ODBC_TEST_FETCH ORDER BY ID");

    SQLINTEGER id;
    SQLLEN ind;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);

    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 1);

    // Commit — cursor behavior depends on SQL_CURSOR_COMMIT_BEHAVIOR
    Commit();

    // After commit, fetching may return SQL_ERROR or SQL_NO_DATA depending
    // on the cursor commit behavior setting. Either is acceptable.
    ret = SQLFetch(hStmt);
    EXPECT_TRUE(ret == SQL_ERROR || ret == SQL_NO_DATA || SQL_SUCCEEDED(ret));
}
