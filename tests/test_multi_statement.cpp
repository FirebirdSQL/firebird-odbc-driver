// tests/test_multi_statement.cpp — Multi-statement handle interleaving (Phase 3.4)

#include "test_helpers.h"

class MultiStatementTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_MULTI",
            "ID INTEGER NOT NULL PRIMARY KEY, VAL VARCHAR(30)");

        for (int i = 1; i <= 5; i++) {
            ReallocStmt();
            char sql[128];
            snprintf(sql, sizeof(sql),
                "INSERT INTO ODBC_TEST_MULTI (ID, VAL) VALUES (%d, 'Val %d')", i, i);
            ExecDirect(sql);
        }
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        // Free extra handles
        for (auto h : extraStmts_) {
            if (h != SQL_NULL_HSTMT) {
                SQLFreeHandle(SQL_HANDLE_STMT, h);
            }
        }
        extraStmts_.clear();
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
    std::vector<SQLHSTMT> extraStmts_;
};

TEST_F(MultiStatementTest, TwoStatementsOnSameConnection) {
    // Allocate a second statement
    SQLHSTMT hStmt2 = AllocExtraStmt();
    extraStmts_.push_back(hStmt2);

    // Execute different queries on each
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_MULTI WHERE ID <= 3 ORDER BY ID", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt2,
        (SQLCHAR*)"SELECT VAL FROM ODBC_TEST_MULTI WHERE ID > 3 ORDER BY ID", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Fetch interleaved
    SQLINTEGER id;
    SQLCHAR val[31];
    SQLLEN idInd, valInd;

    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    SQLBindCol(hStmt2, 1, SQL_C_CHAR, val, sizeof(val), &valInd);

    // Fetch from stmt1
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 1);

    // Fetch from stmt2
    ret = SQLFetch(hStmt2);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)val, "Val 4");

    // Continue interleaved fetching
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 2);

    ret = SQLFetch(hStmt2);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)val, "Val 5");
}

TEST_F(MultiStatementTest, ManySimultaneousHandles) {
    const int NUM_HANDLES = 20;

    // Allocate many statement handles
    for (int i = 0; i < NUM_HANDLES; i++) {
        SQLHSTMT stmt = AllocExtraStmt();
        ASSERT_NE(stmt, (SQLHSTMT)SQL_NULL_HSTMT) << "Failed to allocate handle #" << i;
        extraStmts_.push_back(stmt);
    }

    // Execute a query on each
    for (int i = 0; i < NUM_HANDLES; i++) {
        SQLRETURN ret = SQLExecDirect(extraStmts_[i],
            (SQLCHAR*)"SELECT CURRENT_TIMESTAMP FROM RDB$DATABASE", SQL_NTS);
        ASSERT_TRUE(SQL_SUCCEEDED(ret))
            << "Execute failed on handle #" << i;
    }

    // Fetch from each
    for (int i = 0; i < NUM_HANDLES; i++) {
        SQLRETURN ret = SQLFetch(extraStmts_[i]);
        ASSERT_TRUE(SQL_SUCCEEDED(ret))
            << "Fetch failed on handle #" << i;
    }
}

TEST_F(MultiStatementTest, PrepareAndExecOnDifferentStatements) {
    SQLHSTMT hStmt2 = AllocExtraStmt();
    extraStmts_.push_back(hStmt2);

    // Prepare on stmt1
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT COUNT(*) FROM ODBC_TEST_MULTI WHERE ID > ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER param = 2;
    SQLLEN paramInd = 0;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &param, 0, &paramInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // While stmt1 is prepared, do ad-hoc queries on stmt2
    ret = SQLExecDirect(hStmt2,
        (SQLCHAR*)"SELECT 42 FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER val2;
    SQLLEN ind2;
    SQLBindCol(hStmt2, 1, SQL_C_SLONG, &val2, 0, &ind2);
    ret = SQLFetch(hStmt2);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val2, 42);

    // Now execute the prepared statement
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER count;
    SQLLEN countInd;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, 0, &countInd);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, 3);  // IDs 3, 4, 5
}

// ===== Free handle while others are active =====

TEST_F(MultiStatementTest, FreeOneHandleWhileOthersActive) {
    SQLHSTMT hStmt2 = AllocExtraStmt();
    SQLHSTMT hStmt3 = AllocExtraStmt();
    extraStmts_.push_back(hStmt3);  // only track hStmt3 for cleanup

    // Execute on all three
    SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
    SQLExecDirect(hStmt2,
        (SQLCHAR*)"SELECT 2 FROM RDB$DATABASE", SQL_NTS);
    SQLExecDirect(hStmt3,
        (SQLCHAR*)"SELECT 3 FROM RDB$DATABASE", SQL_NTS);

    // Free hStmt2 while others are active
    SQLRETURN ret = SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
    EXPECT_TRUE(SQL_SUCCEEDED(ret));

    // Other handles should still work
    SQLINTEGER val;
    SQLLEN ind;
    SQLBindCol(hStmt3, 1, SQL_C_SLONG, &val, 0, &ind);
    ret = SQLFetch(hStmt3);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 3);
}
