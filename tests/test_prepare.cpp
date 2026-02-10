// tests/test_prepare.cpp — SQLPrepare/SQLExecute tests
// (Phase 6, ported from psqlodbc prepare-test)
//
// Tests prepared statements with various parameter types, SQLNumResultCols
// before execute, and re-execution with different parameters.

#include "test_helpers.h"
#include <cstring>

class PrepareTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_PREP",
            "ID INTEGER NOT NULL PRIMARY KEY, "
            "VAL_TEXT VARCHAR(100), "
            "VAL_INT INTEGER, "
            "VAL_DOUBLE DOUBLE PRECISION");

        // Insert test data
        ExecDirect("INSERT INTO ODBC_TEST_PREP VALUES (1, 'foo', 10, 1.1)");
        ExecDirect("INSERT INTO ODBC_TEST_PREP VALUES (2, 'bar', 20, 2.2)");
        ExecDirect("INSERT INTO ODBC_TEST_PREP VALUES (3, 'baz', 30, 3.3)");
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

// ===== Basic prepare + execute with text param =====

TEST_F(PrepareTest, PrepareWithTextParam) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT ID, VAL_TEXT FROM ODBC_TEST_PREP WHERE VAL_TEXT = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLPrepare failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    char param[] = "bar";
    SQLLEN cbParam = SQL_NTS;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT,
        SQL_C_CHAR, SQL_CHAR, 20, 0, param, 0, &cbParam);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLExecute failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLINTEGER id = 0;
    SQLCHAR text[32] = {};
    SQLLEN idInd = 0, textInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, text, sizeof(text), &textInd);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 2);
    EXPECT_STREQ((char*)text, "bar");

    // Should be only one row
    ret = SQLFetch(hStmt);
    EXPECT_EQ(ret, SQL_NO_DATA);
}

// ===== SQLNumResultCols before execute =====

TEST_F(PrepareTest, NumResultColsBeforeExecute) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT ID, VAL_TEXT FROM ODBC_TEST_PREP WHERE VAL_TEXT = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Call SQLNumResultCols BEFORE execute — should work
    SQLSMALLINT colCount = 0;
    ret = SQLNumResultCols(hStmt, &colCount);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLNumResultCols failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(colCount, 2);
}

// ===== Prepare with integer param =====

TEST_F(PrepareTest, PrepareWithIntegerParam) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT ID, VAL_TEXT FROM ODBC_TEST_PREP WHERE ID = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER param = 3;
    SQLLEN cbParam = sizeof(param);
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT,
        SQL_C_SLONG, SQL_INTEGER, 0, 0, &param, sizeof(param), &cbParam);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER id = 0;
    SQLCHAR text[32] = {};
    SQLLEN idInd = 0, textInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, text, sizeof(text), &textInd);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 3);
    EXPECT_STREQ((char*)text, "baz");
}

// ===== Re-execute with different parameter =====

TEST_F(PrepareTest, ReExecuteWithDifferentParam) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT VAL_TEXT FROM ODBC_TEST_PREP WHERE ID = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER param = 1;
    SQLLEN cbParam = sizeof(param);
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT,
        SQL_C_SLONG, SQL_INTEGER, 0, 0, &param, sizeof(param), &cbParam);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // First execution: ID = 1
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLCHAR text[32] = {};
    SQLLEN textInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, text, sizeof(text), &textInd);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)text, "foo");

    SQLFreeStmt(hStmt, SQL_CLOSE);

    // Second execution: ID = 2
    param = 2;
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    memset(text, 0, sizeof(text));
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)text, "bar");
}

// ===== Prepare INSERT with parameters =====

TEST_F(PrepareTest, PrepareInsert) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_PREP VALUES (?, ?, ?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER id = 100;
    char text[] = "prepared";
    SQLINTEGER intVal = 999;
    double dblVal = 9.99;
    SQLLEN idInd = sizeof(id), textInd = SQL_NTS;
    SQLLEN intInd = sizeof(intVal), dblInd = sizeof(dblVal);

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
        0, 0, &id, sizeof(id), &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        100, 0, text, 0, &textInd);
    SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
        0, 0, &intVal, sizeof(intVal), &intInd);
    SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,
        0, 0, &dblVal, sizeof(dblVal), &dblInd);

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Execute INSERT failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    Commit();

    // Verify the insert
    ReallocStmt();
    ExecDirect("SELECT VAL_TEXT, VAL_INT, VAL_DOUBLE FROM ODBC_TEST_PREP WHERE ID = 100");

    SQLCHAR readText[32] = {};
    SQLINTEGER readInt = 0;
    double readDbl = 0.0;
    SQLLEN ind1 = 0, ind2 = 0, ind3 = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, readText, sizeof(readText), &ind1);
    SQLBindCol(hStmt, 2, SQL_C_SLONG, &readInt, 0, &ind2);
    SQLBindCol(hStmt, 3, SQL_C_DOUBLE, &readDbl, 0, &ind3);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)readText, "prepared");
    EXPECT_EQ(readInt, 999);
    EXPECT_NEAR(readDbl, 9.99, 0.01);
}

// ===== SQLDescribeCol after prepare =====

TEST_F(PrepareTest, DescribeColAfterPrepare) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT ID, VAL_TEXT, VAL_INT, VAL_DOUBLE FROM ODBC_TEST_PREP WHERE ID = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLSMALLINT colCount = 0;
    ret = SQLNumResultCols(hStmt, &colCount);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(colCount, 4);

    // Describe each column
    for (SQLSMALLINT i = 1; i <= colCount; i++) {
        SQLCHAR colName[64] = {};
        SQLSMALLINT nameLen = 0, dataType = 0, decDigits = 0, nullable = 0;
        SQLULEN colSize = 0;

        ret = SQLDescribeCol(hStmt, i, colName, sizeof(colName), &nameLen,
            &dataType, &colSize, &decDigits, &nullable);
        ASSERT_TRUE(SQL_SUCCEEDED(ret))
            << "SQLDescribeCol(" << i << ") failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
        EXPECT_GT(nameLen, 0) << "Column " << i << " should have a name";
        EXPECT_NE(dataType, 0) << "Column " << i << " should have a data type";
    }
}

// ===== Prepare with BLOB parameter (binary data) =====

TEST_F(PrepareTest, PrepareWithBlobParam) {
    // Create a table with a BLOB column
    ExecIgnoreError("DROP TABLE ODBC_TEST_PREP_BLOB");
    Commit();
    ReallocStmt();
    ExecDirect("CREATE TABLE ODBC_TEST_PREP_BLOB (ID INTEGER NOT NULL PRIMARY KEY, DATA BLOB SUB_TYPE BINARY)");
    Commit();
    ReallocStmt();

    // Prepare insert
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_PREP_BLOB VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Insert binary data
    unsigned char blobData[100];
    for (int i = 0; i < 100; i++) blobData[i] = (unsigned char)i;

    SQLINTEGER id = 1;
    SQLLEN idInd = sizeof(id);
    SQLLEN blobInd = 100;

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
        0, 0, &id, sizeof(id), &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY,
        100, 0, blobData, 100, &blobInd);

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "INSERT blob failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    Commit();

    // Read it back
    ReallocStmt();
    ExecDirect("SELECT DATA FROM ODBC_TEST_PREP_BLOB WHERE ID = 1");

    unsigned char readBuf[128] = {};
    SQLLEN readInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_BINARY, readBuf, sizeof(readBuf), &readInd);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(readInd, 100);
    EXPECT_EQ(memcmp(readBuf, blobData, 100), 0);

    // Cleanup
    SQLCloseCursor(hStmt);
    ExecIgnoreError("DROP TABLE ODBC_TEST_PREP_BLOB");
    Commit();
}

// ===== Multiple parameters =====

TEST_F(PrepareTest, MultipleParamsInWhere) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_PREP WHERE VAL_INT > ? AND VAL_INT < ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER lo = 15, hi = 25;
    SQLLEN loInd = sizeof(lo), hiInd = sizeof(hi);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
        0, 0, &lo, sizeof(lo), &loInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
        0, 0, &hi, sizeof(hi), &hiInd);

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER id = 0;
    SQLLEN idInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 2); // VAL_INT=20 is between 15 and 25

    ret = SQLFetch(hStmt);
    EXPECT_EQ(ret, SQL_NO_DATA);
}

// ===== Prepare without parameters (just statements) =====

TEST_F(PrepareTest, PrepareWithoutParams) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT COUNT(*) FROM ODBC_TEST_PREP", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER count = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, 0, &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, 3);
}

// ===== Varchar param with varying sizes =====

TEST_F(PrepareTest, VarcharParamColumnSize5) {
    // psqlodbc had a special case with column_size=5 and BoolsAsChar=1
    // Verify this works with Firebird
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT ID, VAL_TEXT FROM ODBC_TEST_PREP WHERE ID = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    char param[] = "2";
    SQLLEN cbParam = SQL_NTS;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT,
        SQL_C_CHAR, SQL_VARCHAR, 5, 0, param, 0, &cbParam);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Execute failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLINTEGER id = 0;
    SQLLEN idInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 2);
}
