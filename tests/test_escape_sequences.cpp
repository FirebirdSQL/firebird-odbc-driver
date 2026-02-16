// tests/test_escape_sequences.cpp — Verify ODBC escape sequences are NOT processed
//
// This driver intentionally does NOT process ODBC escape sequences.
// SQL is sent to Firebird as-is for maximum performance and transparency.
// Applications should use native Firebird SQL syntax.

#include "test_helpers.h"

class EscapeSequenceTest : public OdbcConnectedTest {};

// ===== Verify escape sequences are passed through unchanged =====

TEST_F(EscapeSequenceTest, SQLNativeSqlPassesThroughUnchanged) {
    // SQLNativeSql should return the SQL unchanged when escape sequences are not processed
    const char* input = "SELECT {fn UCASE('hello')} FROM RDB$DATABASE";
    SQLCHAR output[512] = {};
    SQLINTEGER outputLen = 0;

    SQLRETURN ret = SQLNativeSql(hDbc,
        (SQLCHAR*)input, SQL_NTS,
        output, sizeof(output), &outputLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLNativeSql failed: " << GetOdbcError(SQL_HANDLE_DBC, hDbc);

    // The output should be the same as input — no escape processing
    EXPECT_GT(outputLen, 0);
    // The braces should still be in the output since we don't process them
    std::string result((char*)output, outputLen);
    EXPECT_NE(result.find('{'), std::string::npos)
        << "Escape braces were unexpectedly removed from: " << result;
}

TEST_F(EscapeSequenceTest, SQLNativeSqlPlainSqlUnchanged) {
    // Plain SQL without escapes should pass through unchanged
    const char* input = "SELECT UPPER('hello') FROM RDB$DATABASE";
    SQLCHAR output[512] = {};
    SQLINTEGER outputLen = 0;

    SQLRETURN ret = SQLNativeSql(hDbc,
        (SQLCHAR*)input, SQL_NTS,
        output, sizeof(output), &outputLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLNativeSql failed: " << GetOdbcError(SQL_HANDLE_DBC, hDbc);

    EXPECT_EQ(std::string((char*)output, outputLen), std::string(input));
}

// ===== Verify native Firebird functions work directly =====

TEST_F(EscapeSequenceTest, NativeUpperFunction) {
    // Use native Firebird UPPER() instead of {fn UCASE()}
    ExecDirect("SELECT UPPER('hello') FROM RDB$DATABASE");

    SQLCHAR val[32] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)val, "HELLO");
}

TEST_F(EscapeSequenceTest, NativeLowerFunction) {
    ExecDirect("SELECT LOWER('HELLO') FROM RDB$DATABASE");

    SQLCHAR val[32] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)val, "hello");
}

TEST_F(EscapeSequenceTest, NativeConcatOperator) {
    // Use native Firebird || operator instead of {fn CONCAT()}
    ExecDirect("SELECT 'Hello' || ' ' || 'World' FROM RDB$DATABASE");

    SQLCHAR val[64] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)val, "Hello World");
}

TEST_F(EscapeSequenceTest, NativeDateLiteral) {
    // Use native Firebird DATE literal instead of {d '...'}
    ExecDirect("SELECT DATE '2025-06-15' FROM RDB$DATABASE");

    SQL_DATE_STRUCT val = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_TYPE_DATE, &val, sizeof(val), &ind);
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val.year, 2025);
    EXPECT_EQ(val.month, 6);
    EXPECT_EQ(val.day, 15);
}

TEST_F(EscapeSequenceTest, NativeTimestampLiteral) {
    // Use native Firebird TIMESTAMP literal instead of {ts '...'}
    ExecDirect("SELECT TIMESTAMP '2025-12-31 23:59:59' FROM RDB$DATABASE");

    SQL_TIMESTAMP_STRUCT val = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_TYPE_TIMESTAMP, &val, sizeof(val), &ind);
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val.year, 2025);
    EXPECT_EQ(val.month, 12);
    EXPECT_EQ(val.day, 31);
    EXPECT_EQ(val.hour, 23);
    EXPECT_EQ(val.minute, 59);
    EXPECT_EQ(val.second, 59);
}

TEST_F(EscapeSequenceTest, NativeOuterJoin) {
    // Use native Firebird LEFT OUTER JOIN instead of {oj ...}
    ExecIgnoreError("DROP TABLE ODBC_TEST_OJ_A");
    ExecIgnoreError("DROP TABLE ODBC_TEST_OJ_B");
    Commit();
    ReallocStmt();

    ExecDirect("CREATE TABLE ODBC_TEST_OJ_A (ID INTEGER NOT NULL PRIMARY KEY)");
    Commit();
    ReallocStmt();
    ExecDirect("CREATE TABLE ODBC_TEST_OJ_B (ID INTEGER NOT NULL PRIMARY KEY, A_ID INTEGER)");
    Commit();
    ReallocStmt();

    ExecDirect("INSERT INTO ODBC_TEST_OJ_A (ID) VALUES (1)");
    ExecDirect("INSERT INTO ODBC_TEST_OJ_A (ID) VALUES (2)");
    Commit();
    ReallocStmt();

    ExecDirect("INSERT INTO ODBC_TEST_OJ_B (ID, A_ID) VALUES (10, 1)");
    Commit();
    ReallocStmt();

    // Native Firebird JOIN syntax — no escape braces
    ExecDirect("SELECT A.ID, B.ID FROM ODBC_TEST_OJ_A A "
               "LEFT OUTER JOIN ODBC_TEST_OJ_B B ON A.ID = B.A_ID "
               "ORDER BY A.ID");

    SQLINTEGER aId, bId;
    SQLLEN aInd, bInd;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &aId, 0, &aInd);
    SQLBindCol(hStmt, 2, SQL_C_SLONG, &bId, 0, &bInd);

    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(aId, 1);
    EXPECT_EQ(bId, 10);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(aId, 2);
    EXPECT_EQ(bInd, SQL_NULL_DATA);

    SQLCloseCursor(hStmt);
    ExecIgnoreError("DROP TABLE ODBC_TEST_OJ_B");
    ExecIgnoreError("DROP TABLE ODBC_TEST_OJ_A");
    Commit();
}

// ===== Verify SQLGetInfo reports no escape support =====

TEST_F(EscapeSequenceTest, GetInfoNoNumericFunctions) {
    SQLUINTEGER val = 0xFFFFFFFF;
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_NUMERIC_FUNCTIONS, &val, sizeof(val), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 0u) << "SQL_NUMERIC_FUNCTIONS should be 0 (no escape processing)";
}

TEST_F(EscapeSequenceTest, GetInfoNoStringFunctions) {
    SQLUINTEGER val = 0xFFFFFFFF;
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_STRING_FUNCTIONS, &val, sizeof(val), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 0u) << "SQL_STRING_FUNCTIONS should be 0 (no escape processing)";
}

TEST_F(EscapeSequenceTest, GetInfoNoTimedateFunctions) {
    SQLUINTEGER val = 0xFFFFFFFF;
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_TIMEDATE_FUNCTIONS, &val, sizeof(val), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 0u) << "SQL_TIMEDATE_FUNCTIONS should be 0 (no escape processing)";
}

TEST_F(EscapeSequenceTest, GetInfoNoSystemFunctions) {
    SQLUINTEGER val = 0xFFFFFFFF;
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_SYSTEM_FUNCTIONS, &val, sizeof(val), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 0u) << "SQL_SYSTEM_FUNCTIONS should be 0 (no escape processing)";
}

TEST_F(EscapeSequenceTest, GetInfoConvertFunctionsCastOnly) {
    SQLUINTEGER val = 0;
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_CONVERT_FUNCTIONS, &val, sizeof(val), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    // Only CAST is reported (native SQL, not an escape)
    EXPECT_EQ(val, (SQLUINTEGER)SQL_FN_CVT_CAST)
        << "SQL_CONVERT_FUNCTIONS should only report CAST";
}
