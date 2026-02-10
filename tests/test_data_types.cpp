// tests/test_data_types.cpp — Data type and conversion tests (Phase 3.6, 3.7)

#include "test_helpers.h"
#include <cmath>
#include <climits>

class DataTypeTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_TYPES",
            "ID INTEGER NOT NULL PRIMARY KEY, "
            "COL_SMALLINT SMALLINT, "
            "COL_INTEGER INTEGER, "
            "COL_BIGINT BIGINT, "
            "COL_FLOAT FLOAT, "
            "COL_DOUBLE DOUBLE PRECISION, "
            "COL_NUMERIC NUMERIC(18,4), "
            "COL_DECIMAL DECIMAL(9,2), "
            "COL_VARCHAR VARCHAR(100), "
            "COL_CHAR CHAR(20), "
            "COL_DATE DATE, "
            "COL_TIME TIME, "
            "COL_TIMESTAMP TIMESTAMP, "
            "COL_BLOB BLOB SUB_TYPE TEXT"
        );
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

// ===== Integer types =====

TEST_F(DataTypeTest, SmallintRoundTrip) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_SMALLINT) VALUES (1, 32000)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_SMALLINT FROM ODBC_TEST_TYPES WHERE ID = 1");

    SQLSMALLINT val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SSHORT, &val, 0, &ind);
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 32000);
}

TEST_F(DataTypeTest, IntegerRoundTrip) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_INTEGER) VALUES (2, 2147483647)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_INTEGER FROM ODBC_TEST_TYPES WHERE ID = 2");

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(val, 2147483647);
}

TEST_F(DataTypeTest, BigintRoundTrip) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_BIGINT) VALUES (3, 9223372036854775807)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_BIGINT FROM ODBC_TEST_TYPES WHERE ID = 3");

    SQLBIGINT val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SBIGINT, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(val, INT64_C(9223372036854775807));
}

// ===== Floating point =====

TEST_F(DataTypeTest, FloatRoundTrip) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_FLOAT) VALUES (4, 3.14)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_FLOAT FROM ODBC_TEST_TYPES WHERE ID = 4");

    float val = 0.0f;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_FLOAT, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_NEAR(val, 3.14f, 0.01f);
}

TEST_F(DataTypeTest, DoubleRoundTrip) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_DOUBLE) VALUES (5, 2.718281828459045)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_DOUBLE FROM ODBC_TEST_TYPES WHERE ID = 5");

    double val = 0.0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_DOUBLE, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_NEAR(val, 2.718281828459045, 1e-12);
}

// ===== NUMERIC / DECIMAL precision tests =====

TEST_F(DataTypeTest, NumericPrecision) {
    // NUMERIC(18,4) should preserve 4 decimal places
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_NUMERIC) VALUES (6, 12345678901234.5678)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_NUMERIC FROM ODBC_TEST_TYPES WHERE ID = 6");

    // Read as string to avoid floating point approximation
    SQLCHAR val[64] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    // Should contain "12345678901234.5678" (or close to it)
    double dval = atof((char*)val);
    EXPECT_NEAR(dval, 12345678901234.5678, 0.001);
}

TEST_F(DataTypeTest, DecimalNegative) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_DECIMAL) VALUES (7, -1234567.89)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_DECIMAL FROM ODBC_TEST_TYPES WHERE ID = 7");

    double val = 0.0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_DOUBLE, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_NEAR(val, -1234567.89, 0.01);
}

TEST_F(DataTypeTest, NumericZero) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_NUMERIC) VALUES (8, 0.0000)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_NUMERIC FROM ODBC_TEST_TYPES WHERE ID = 8");

    double val = -1.0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_DOUBLE, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(val, 0.0);
}

// ===== String types =====

TEST_F(DataTypeTest, VarcharRoundTrip) {
    ReallocStmt();

    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_TYPES (ID, COL_VARCHAR) VALUES (9, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    const char* testStr = "Hello, Firebird ODBC!";
    SQLLEN strInd = SQL_NTS;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
                           100, 0, (SQLPOINTER)testStr, 0, &strInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_VARCHAR FROM ODBC_TEST_TYPES WHERE ID = 9");

    SQLCHAR val[101] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_STREQ((char*)val, testStr);
}

TEST_F(DataTypeTest, CharPadding) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_CHAR) VALUES (10, 'ABC')");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_CHAR FROM ODBC_TEST_TYPES WHERE ID = 10");

    SQLCHAR val[21] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    // CHAR(20) should be padded with spaces
    EXPECT_EQ(strlen((char*)val), 20u);
    EXPECT_EQ(val[0], 'A');
    EXPECT_EQ(val[1], 'B');
    EXPECT_EQ(val[2], 'C');
    EXPECT_EQ(val[3], ' ');
}

// ===== NULL handling =====

TEST_F(DataTypeTest, NullValue) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_INTEGER) VALUES (11, NULL)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_INTEGER FROM ODBC_TEST_TYPES WHERE ID = 11");

    SQLINTEGER val = 42;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(ind, SQL_NULL_DATA);
}

// ===== Date/Time types =====

TEST_F(DataTypeTest, DateRoundTrip) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_DATE) VALUES (12, '2025-06-15')");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_DATE FROM ODBC_TEST_TYPES WHERE ID = 12");

    SQL_DATE_STRUCT val = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_TYPE_DATE, &val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(val.year, 2025);
    EXPECT_EQ(val.month, 6);
    EXPECT_EQ(val.day, 15);
}

TEST_F(DataTypeTest, TimestampRoundTrip) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_TIMESTAMP) VALUES (13, '2025-12-31 23:59:59')");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_TIMESTAMP FROM ODBC_TEST_TYPES WHERE ID = 13");

    SQL_TIMESTAMP_STRUCT val = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_TYPE_TIMESTAMP, &val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(val.year, 2025);
    EXPECT_EQ(val.month, 12);
    EXPECT_EQ(val.day, 31);
    EXPECT_EQ(val.hour, 23);
    EXPECT_EQ(val.minute, 59);
    EXPECT_EQ(val.second, 59);
}

// ===== Cross-type conversions =====
// (IntegerToString and StringTruncation tests removed — duplicated by
//  test_result_conversions.cpp IntToChar and CharTruncation)

TEST_F(DataTypeTest, StringToInteger) {
    // Insert a string value, read as integer via CAST
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_VARCHAR) VALUES (15, '12345')");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT CAST(COL_VARCHAR AS INTEGER) FROM ODBC_TEST_TYPES WHERE ID = 15");

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(val, 12345);
}

// ===== SQLGetData (unbounded fetch) =====

TEST_F(DataTypeTest, GetDataInteger) {
    ExecDirect("INSERT INTO ODBC_TEST_TYPES (ID, COL_INTEGER) VALUES (16, 999)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT COL_INTEGER FROM ODBC_TEST_TYPES WHERE ID = 16");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 999);
}

// (GetDataStringTruncation test removed — duplicated by
//  test_result_conversions.cpp CharTruncation)

// ===== Parameter binding =====

TEST_F(DataTypeTest, ParameterizedInsertAndSelect) {
    ReallocStmt();

    // Prepare parameterized insert
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_TYPES (ID, COL_INTEGER, COL_VARCHAR) VALUES (?, ?, ?)",
        SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER id = 18;
    SQLINTEGER intVal = 777;
    SQLCHAR strVal[] = "Parameterized";
    SQLLEN idInd = 0, intInd = 0, strInd = SQL_NTS;

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id, 0, &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &intVal, 0, &intInd);
    SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 100, 0, strVal, 0, &strInd);

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Parameterized insert failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    Commit();
    ReallocStmt();

    // Parameterized select
    ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT COL_INTEGER, COL_VARCHAR FROM ODBC_TEST_TYPES WHERE ID = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER paramId = 18;
    SQLLEN paramIdInd = 0;
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &paramId, 0, &paramIdInd);

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER resultInt = 0;
    SQLCHAR resultStr[101] = {};
    SQLLEN resultIntInd = 0, resultStrInd = 0;

    SQLBindCol(hStmt, 1, SQL_C_SLONG, &resultInt, 0, &resultIntInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, resultStr, sizeof(resultStr), &resultStrInd);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(resultInt, 777);
    EXPECT_STREQ((char*)resultStr, "Parameterized");
}
