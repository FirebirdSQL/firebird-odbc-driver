// tests/test_bind_cycle.cpp — Bind/unbind cycling and column rebinding (Phase 3.10)

#include "test_helpers.h"

class BindCycleTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_BIND",
            "ID INTEGER NOT NULL PRIMARY KEY, "
            "A INTEGER, B VARCHAR(20), C DOUBLE PRECISION"
        );

        ExecDirect("INSERT INTO ODBC_TEST_BIND (ID, A, B, C) VALUES (1, 10, 'alpha', 1.1)");
        ExecDirect("INSERT INTO ODBC_TEST_BIND (ID, A, B, C) VALUES (2, 20, 'beta', 2.2)");
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

TEST_F(BindCycleTest, RebindColumnBetweenExecutions) {
    // First execution: bind col 1 as integer
    ExecDirect("SELECT A FROM ODBC_TEST_BIND WHERE ID = 1");

    SQLINTEGER intVal = 0;
    SQLLEN intInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &intVal, 0, &intInd);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(intVal, 10);

    // Close cursor, rebind as string
    SQLCloseCursor(hStmt);
    ExecDirect("SELECT A FROM ODBC_TEST_BIND WHERE ID = 2");

    SQLCHAR strVal[32] = {};
    SQLLEN strInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, strVal, sizeof(strVal), &strInd);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_STREQ((char*)strVal, "20");
}

TEST_F(BindCycleTest, UnbindAllColumns) {
    ExecDirect("SELECT A, B FROM ODBC_TEST_BIND WHERE ID = 1");

    SQLINTEGER a = 0;
    SQLCHAR b[21] = {};
    SQLLEN aInd = 0, bInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &a, 0, &aInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, b, sizeof(b), &bInd);

    // Unbind all by setting SQL_UNBIND
    SQLRETURN ret = SQLFreeStmt(hStmt, SQL_UNBIND);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // After unbinding, fetch should work but bound variables shouldn't be modified
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(a, 0);  // should NOT have been written to

    // But GetData should still work
    SQLINTEGER getVal = 0;
    SQLLEN getInd = 0;
    ret = SQLGetData(hStmt, 1, SQL_C_SLONG, &getVal, 0, &getInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(getVal, 10);
}

TEST_F(BindCycleTest, ResetParameters) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT A FROM ODBC_TEST_BIND WHERE ID = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER param = 1;
    SQLLEN paramInd = 0;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &param, 0, &paramInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Reset parameters
    ret = SQLFreeStmt(hStmt, SQL_RESET_PARAMS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Rebind with different value
    SQLINTEGER param2 = 2;
    SQLLEN param2Ind = 0;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &param2, 0, &param2Ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER result = 0;
    SQLLEN resultInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &result, 0, &resultInd);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(result, 20);
}

TEST_F(BindCycleTest, PrepareExecuteRepeatWithDifferentParams) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT B FROM ODBC_TEST_BIND WHERE ID = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER param = 0;
    SQLLEN paramInd = 0;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &param, 0, &paramInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLCHAR result[21] = {};
    SQLLEN resultInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, result, sizeof(result), &resultInd);

    // Execute with param=1
    param = 1;
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)result, "alpha");

    // Close cursor and re-execute with param=2
    SQLCloseCursor(hStmt);
    param = 2;
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)result, "beta");
}
