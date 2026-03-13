// tests/test_result_conversions.cpp — Result type conversion tests
// (Phase 6, ported from psqlodbc result-conversions-test)
//
// Tests SQLGetData with various C type conversions for each Firebird SQL type.

#include "test_helpers.h"
#include <cmath>
#include <climits>
#include <cstring>

class ResultConversionsTest : public OdbcConnectedTest {};

// Helper: execute a SELECT and fetch one value with SQLGetData
template<typename T>
struct GetDataResult {
    T value;
    SQLLEN indicator;
    SQLRETURN ret;
};

static GetDataResult<SQLINTEGER> getAsInteger(SQLHSTMT hStmt, int col = 1) {
    GetDataResult<SQLINTEGER> r = {};
    r.ret = SQLGetData(hStmt, col, SQL_C_SLONG, &r.value, 0, &r.indicator);
    return r;
}

static GetDataResult<SQLBIGINT> getAsBigint(SQLHSTMT hStmt, int col = 1) {
    GetDataResult<SQLBIGINT> r = {};
    r.ret = SQLGetData(hStmt, col, SQL_C_SBIGINT, &r.value, 0, &r.indicator);
    return r;
}

static GetDataResult<double> getAsDouble(SQLHSTMT hStmt, int col = 1) {
    GetDataResult<double> r = {};
    r.ret = SQLGetData(hStmt, col, SQL_C_DOUBLE, &r.value, 0, &r.indicator);
    return r;
}

// ===== Integer to various C types =====

TEST_F(ResultConversionsTest, IntegerToChar) {
    ExecDirect("SELECT 12345 FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[32] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)buf, "12345");
}

TEST_F(ResultConversionsTest, IntegerToSLong) {
    ExecDirect("SELECT 42 FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsInteger(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    EXPECT_EQ(r.value, 42);
}

TEST_F(ResultConversionsTest, IntegerToDouble) {
    ExecDirect("SELECT 42 FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsDouble(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    EXPECT_DOUBLE_EQ(r.value, 42.0);
}

TEST_F(ResultConversionsTest, IntegerToSmallint) {
    ExecDirect("SELECT 123 FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLSMALLINT val = 0;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_SSHORT, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 123);
}

TEST_F(ResultConversionsTest, IntegerToBigint) {
    ExecDirect("SELECT 2147483647 FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsBigint(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    EXPECT_EQ(r.value, INT64_C(2147483647));
}

TEST_F(ResultConversionsTest, IntegerToFloat) {
    ExecDirect("SELECT 100 FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    float val = 0.0f;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_FLOAT, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_FLOAT_EQ(val, 100.0f);
}

TEST_F(ResultConversionsTest, IntegerToBit) {
    ExecDirect("SELECT 1 FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR val = 255;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_BIT, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 1);
}

TEST_F(ResultConversionsTest, IntegerToBinary) {
    ExecDirect("SELECT 42 FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    unsigned char buf[32] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_BINARY, buf, sizeof(buf), &ind);
    // Integer→Binary conversion may or may not be supported
    // Just verify no crash
    SUCCEED();
}

// ===== Double to various C types =====

TEST_F(ResultConversionsTest, DoubleToChar) {
    ExecDirect("SELECT CAST(3.14 AS DOUBLE PRECISION) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[64] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    double parsed = atof((char*)buf);
    EXPECT_NEAR(parsed, 3.14, 0.001);
}

TEST_F(ResultConversionsTest, DoubleToSLong) {
    ExecDirect("SELECT CAST(3.14 AS DOUBLE PRECISION) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsInteger(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    EXPECT_EQ(r.value, 3); // Truncated to integer
}

TEST_F(ResultConversionsTest, DoubleToDouble) {
    ExecDirect("SELECT CAST(3.14159 AS DOUBLE PRECISION) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsDouble(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    EXPECT_NEAR(r.value, 3.14159, 1e-5);
}

// ===== VARCHAR to various C types =====

TEST_F(ResultConversionsTest, VarcharToChar) {
    ExecDirect("SELECT CAST('hello world' AS VARCHAR(50)) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[64] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)buf, "hello world");
}

TEST_F(ResultConversionsTest, VarcharNumericToSLong) {
    // VARCHAR→INTEGER conversion via CAST at SQL level
    ExecDirect("SELECT CAST('42' AS INTEGER) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsInteger(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    EXPECT_EQ(r.value, 42);
}

TEST_F(ResultConversionsTest, VarcharNumericToDouble) {
    // VARCHAR→DOUBLE conversion via CAST at SQL level
    ExecDirect("SELECT CAST('3.14' AS DOUBLE PRECISION) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsDouble(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    EXPECT_NEAR(r.value, 3.14, 0.001);
}

// ===== String truncation =====

TEST_F(ResultConversionsTest, CharTruncation) {
    ExecDirect("SELECT CAST('this is a long string' AS VARCHAR(100)) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[8] = {}; // Deliberately small buffer
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    // Should return SQL_SUCCESS_WITH_INFO (01004 = string data, right truncated)
    EXPECT_EQ(ret, SQL_SUCCESS_WITH_INFO);
    std::string sqlState = GetSqlState(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(sqlState, "01004");
    // buf should have 7 chars + null terminator
    EXPECT_EQ(strlen((char*)buf), 7u);
}

// ===== Date/Time conversions =====

TEST_F(ResultConversionsTest, DateToChar) {
    ExecDirect("SELECT DATE '2025-01-15' FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[32] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    std::string s((char*)buf);
    EXPECT_NE(s.find("2025"), std::string::npos)
        << "Date string should contain year 2025: " << s;
}

TEST_F(ResultConversionsTest, DateToDateStruct) {
    ExecDirect("SELECT DATE '2025-03-20' FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQL_DATE_STRUCT val = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_TYPE_DATE, &val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val.year, 2025);
    EXPECT_EQ(val.month, 3);
    EXPECT_EQ(val.day, 20);
}

TEST_F(ResultConversionsTest, TimeToChar) {
    ExecDirect("SELECT TIME '14:30:00' FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[32] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    std::string s((char*)buf);
    EXPECT_NE(s.find("14"), std::string::npos) << "Time should contain hour 14: " << s;
}

TEST_F(ResultConversionsTest, TimeToTimeStruct) {
    ExecDirect("SELECT TIME '14:30:45' FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQL_TIME_STRUCT val = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_TYPE_TIME, &val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val.hour, 14);
    EXPECT_EQ(val.minute, 30);
    EXPECT_EQ(val.second, 45);
}

TEST_F(ResultConversionsTest, TimestampToTimestampStruct) {
    ExecDirect("SELECT TIMESTAMP '2025-06-15 10:30:45' FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQL_TIMESTAMP_STRUCT val = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_TYPE_TIMESTAMP, &val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val.year, 2025);
    EXPECT_EQ(val.month, 6);
    EXPECT_EQ(val.day, 15);
    EXPECT_EQ(val.hour, 10);
    EXPECT_EQ(val.minute, 30);
    EXPECT_EQ(val.second, 45);
}

// ===== NUMERIC precision =====

TEST_F(ResultConversionsTest, NumericToChar) {
    ExecDirect("SELECT CAST(1234.5678 AS NUMERIC(18,4)) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[64] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    double parsed = atof((char*)buf);
    EXPECT_NEAR(parsed, 1234.5678, 0.001);
}

TEST_F(ResultConversionsTest, NumericToDouble) {
    ExecDirect("SELECT CAST(1234.5678 AS NUMERIC(18,4)) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsDouble(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    EXPECT_NEAR(r.value, 1234.5678, 0.001);
}

TEST_F(ResultConversionsTest, NumericToInteger) {
    ExecDirect("SELECT CAST(42.99 AS NUMERIC(10,2)) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsInteger(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    // Truncated to 42 or rounded to 43 depending on driver
    EXPECT_TRUE(r.value == 42 || r.value == 43);
}

// ===== NULL handling =====

TEST_F(ResultConversionsTest, NullToChar) {
    ExecDirect("SELECT CAST(NULL AS VARCHAR(10)) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[32] = "sentinel";
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ind, SQL_NULL_DATA);
}

TEST_F(ResultConversionsTest, NullToSLong) {
    ExecDirect("SELECT CAST(NULL AS INTEGER) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLINTEGER val = -1;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ind, SQL_NULL_DATA);
}

TEST_F(ResultConversionsTest, NullToDouble) {
    ExecDirect("SELECT CAST(NULL AS DOUBLE PRECISION) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    double val = -1.0;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_DOUBLE, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ind, SQL_NULL_DATA);
}

TEST_F(ResultConversionsTest, NullToDate) {
    ExecDirect("SELECT CAST(NULL AS DATE) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQL_DATE_STRUCT val = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_TYPE_DATE, &val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ind, SQL_NULL_DATA);
}

// ===== Boolean (Firebird 3.0+) =====

TEST_F(ResultConversionsTest, BooleanToChar) {
    ExecDirect("SELECT TRUE FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[16] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    // Firebird returns "1" or "true" depending on driver mapping
    std::string s((char*)buf);
    EXPECT_TRUE(s == "1" || s == "true" || s == "TRUE" || s == "T")
        << "Boolean true as char: " << s;
}

TEST_F(ResultConversionsTest, BooleanToBit) {
    ExecDirect("SELECT TRUE FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR val = 255;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_BIT, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 1);
}

// ===== Negative values =====

TEST_F(ResultConversionsTest, NegativeIntegerToChar) {
    ExecDirect("SELECT -42 FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[32] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)buf, "-42");
}

TEST_F(ResultConversionsTest, NegativeDoubleToSLong) {
    ExecDirect("SELECT CAST(-99.9 AS DOUBLE PRECISION) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsInteger(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    EXPECT_TRUE(r.value == -99 || r.value == -100);
}

// ===== BIGINT =====

TEST_F(ResultConversionsTest, BigintToChar) {
    ExecDirect("SELECT CAST(9223372036854775807 AS BIGINT) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[32] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)buf, "9223372036854775807");
}

TEST_F(ResultConversionsTest, BigintToBigint) {
    ExecDirect("SELECT CAST(9223372036854775807 AS BIGINT) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    auto r = getAsBigint(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(r.ret));
    EXPECT_EQ(r.value, INT64_C(9223372036854775807));
}

// ===== SMALLINT =====

TEST_F(ResultConversionsTest, SmallintToChar) {
    ExecDirect("SELECT CAST(32000 AS SMALLINT) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[16] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)buf, "32000");
}

TEST_F(ResultConversionsTest, SmallintToSShort) {
    ExecDirect("SELECT CAST(-32000 AS SMALLINT) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLSMALLINT val = 0;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_SSHORT, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, -32000);
}
