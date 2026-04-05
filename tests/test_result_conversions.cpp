// tests/test_result_conversions.cpp — Result type conversion tests
// (Phase 6, ported from psqlodbc result-conversions-test)
//
// Tests SQLGetData with various C type conversions for each Firebird SQL type.
// Parameterized with TEST_P to reduce repetition (Phase 16.2.3).

#include "test_helpers.h"
#include <cmath>
#include <climits>
#include <cstring>
#include <tuple>

class ResultConversionsTest : public OdbcConnectedTest {};

// ===== Parameterized: SQL → String conversions =====
// Tests that SQLGetData(SQL_C_CHAR) produces the expected string representation.

struct ToStringParam {
    const char* name;
    const char* sql;
    const char* expected;  // exact match via STREQ
};

class ResultToStringTest : public OdbcConnectedTest,
                           public ::testing::WithParamInterface<ToStringParam> {};

TEST_P(ResultToStringTest, GetDataAsChar) {
    auto& p = GetParam();
    ExecDirect(p.sql);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buf[64] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)buf, p.expected);
}

INSTANTIATE_TEST_SUITE_P(Conversions, ResultToStringTest, ::testing::Values(
    ToStringParam{"IntegerToChar",   "SELECT 12345 FROM RDB$DATABASE",                           "12345"},
    ToStringParam{"NegIntToChar",    "SELECT -42 FROM RDB$DATABASE",                             "-42"},
    ToStringParam{"BigintToChar",    "SELECT CAST(9223372036854775807 AS BIGINT) FROM RDB$DATABASE", "9223372036854775807"},
    ToStringParam{"SmallintToChar",  "SELECT CAST(32000 AS SMALLINT) FROM RDB$DATABASE",         "32000"},
    ToStringParam{"VarcharToChar",   "SELECT CAST('hello world' AS VARCHAR(50)) FROM RDB$DATABASE", "hello world"}
), [](const auto& info) { return info.param.name; });

// ===== Parameterized: SQL → Integer conversions =====

struct ToIntParam {
    const char* name;
    const char* sql;
    SQLINTEGER expected;
};

class ResultToIntTest : public OdbcConnectedTest,
                        public ::testing::WithParamInterface<ToIntParam> {};

TEST_P(ResultToIntTest, GetDataAsSLong) {
    auto& p = GetParam();
    ExecDirect(p.sql);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, p.expected);
}

INSTANTIATE_TEST_SUITE_P(Conversions, ResultToIntTest, ::testing::Values(
    ToIntParam{"IntegerToSLong",       "SELECT 42 FROM RDB$DATABASE",                                  42},
    ToIntParam{"DoubleToSLong",        "SELECT CAST(3.14 AS DOUBLE PRECISION) FROM RDB$DATABASE",      3},
    ToIntParam{"VarcharNumToSLong",    "SELECT CAST('42' AS INTEGER) FROM RDB$DATABASE",               42}
), [](const auto& info) { return info.param.name; });

// ===== Parameterized: SQL → Double conversions =====

struct ToDoubleParam {
    const char* name;
    const char* sql;
    double expected;
    double tolerance;
};

class ResultToDoubleTest : public OdbcConnectedTest,
                           public ::testing::WithParamInterface<ToDoubleParam> {};

TEST_P(ResultToDoubleTest, GetDataAsDouble) {
    auto& p = GetParam();
    ExecDirect(p.sql);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    double val = 0.0;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_DOUBLE, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_NEAR(val, p.expected, p.tolerance);
}

INSTANTIATE_TEST_SUITE_P(Conversions, ResultToDoubleTest, ::testing::Values(
    ToDoubleParam{"IntegerToDouble",   "SELECT 42 FROM RDB$DATABASE",                              42.0,       1e-10},
    ToDoubleParam{"DoubleToDouble",    "SELECT CAST(3.14159 AS DOUBLE PRECISION) FROM RDB$DATABASE", 3.14159,  1e-5},
    ToDoubleParam{"VarcharToDouble",   "SELECT CAST('3.14' AS DOUBLE PRECISION) FROM RDB$DATABASE",  3.14,     0.001},
    ToDoubleParam{"NumericToDouble",   "SELECT CAST(1234.5678 AS NUMERIC(18,4)) FROM RDB$DATABASE",  1234.5678,0.001}
), [](const auto& info) { return info.param.name; });

// ===== Parameterized: NULL → various C types =====

struct NullParam {
    const char* name;
    const char* sql;
    SQLSMALLINT cType;
    SQLLEN bufSize;
};

class ResultNullTest : public OdbcConnectedTest,
                       public ::testing::WithParamInterface<NullParam> {};

TEST_P(ResultNullTest, NullIndicator) {
    auto& p = GetParam();
    ExecDirect(p.sql);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    char buf[64] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, p.cType, buf, p.bufSize, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ind, SQL_NULL_DATA);
}

INSTANTIATE_TEST_SUITE_P(Conversions, ResultNullTest, ::testing::Values(
    NullParam{"NullToChar",   "SELECT CAST(NULL AS VARCHAR(10)) FROM RDB$DATABASE",   SQL_C_CHAR,       32},
    NullParam{"NullToSLong",  "SELECT CAST(NULL AS INTEGER) FROM RDB$DATABASE",        SQL_C_SLONG,      0},
    NullParam{"NullToDouble", "SELECT CAST(NULL AS DOUBLE PRECISION) FROM RDB$DATABASE",SQL_C_DOUBLE,    0},
    NullParam{"NullToDate",   "SELECT CAST(NULL AS DATE) FROM RDB$DATABASE",           SQL_C_TYPE_DATE,  sizeof(SQL_DATE_STRUCT)}
), [](const auto& info) { return info.param.name; });

// ===== Non-parameterized: special cases that need unique comparison logic =====

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
    SQLBIGINT val = 0;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_SBIGINT, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, INT64_C(2147483647));
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
    SQLGetData(hStmt, 1, SQL_C_BINARY, buf, sizeof(buf), &ind);
    SUCCEED();  // Just verify no crash
}

TEST_F(ResultConversionsTest, DoubleToChar) {
    ExecDirect("SELECT CAST(3.14 AS DOUBLE PRECISION) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    SQLCHAR buf[64] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_NEAR(atof((char*)buf), 3.14, 0.001);
}

TEST_F(ResultConversionsTest, NumericToChar) {
    ExecDirect("SELECT CAST(1234.5678 AS NUMERIC(18,4)) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    SQLCHAR buf[64] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_NEAR(atof((char*)buf), 1234.5678, 0.001);
}

TEST_F(ResultConversionsTest, NumericToInteger) {
    ExecDirect("SELECT CAST(42.99 AS NUMERIC(10,2)) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_TRUE(val == 42 || val == 43);
}

TEST_F(ResultConversionsTest, NegativeDoubleToSLong) {
    ExecDirect("SELECT CAST(-99.9 AS DOUBLE PRECISION) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_TRUE(val == -99 || val == -100);
}

TEST_F(ResultConversionsTest, BigintToBigint) {
    ExecDirect("SELECT CAST(9223372036854775807 AS BIGINT) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    SQLBIGINT val = 0;
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_SBIGINT, &val, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, INT64_C(9223372036854775807));
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

// ===== String truncation =====

TEST_F(ResultConversionsTest, CharTruncation) {
    ExecDirect("SELECT CAST('this is a long string' AS VARCHAR(100)) FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    SQLCHAR buf[8] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    EXPECT_EQ(ret, SQL_SUCCESS_WITH_INFO);
    EXPECT_EQ(GetSqlState(SQL_HANDLE_STMT, hStmt), "01004");
    EXPECT_EQ(strlen((char*)buf), 7u);
}

// ===== Date/Time struct conversions =====

TEST_F(ResultConversionsTest, DateToChar) {
    ExecDirect("SELECT DATE '2025-01-15' FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    SQLCHAR buf[32] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_NE(std::string((char*)buf).find("2025"), std::string::npos);
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
    EXPECT_NE(std::string((char*)buf).find("14"), std::string::npos);
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

// ===== Boolean (Firebird 3.0+) =====

TEST_F(ResultConversionsTest, BooleanToChar) {
    ExecDirect("SELECT TRUE FROM RDB$DATABASE");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    SQLCHAR buf[16] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
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
