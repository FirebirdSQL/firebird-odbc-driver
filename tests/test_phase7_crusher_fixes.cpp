// tests/test_phase7_crusher_fixes.cpp — Tests for Phase 7 ODBC Crusher-identified bug fixes
//
// OC-1: SQLCopyDesc crash on empty descriptor
// OC-2: SQL_DIAG_ROW_COUNT always returns 0
// OC-3: SQL_ATTR_CONNECTION_TIMEOUT not supported
// OC-4: SQL_ATTR_ASYNC_ENABLE silently accepted
// OC-5: returnStringInfo reports truncated length instead of full length

#include "test_helpers.h"

// ===== OC-1: SQLCopyDesc crash on empty descriptor =====
class CopyDescCrashTest : public OdbcConnectedTest {};

TEST_F(CopyDescCrashTest, CopyEmptyARDDoesNotCrash) {
    // Allocate two statements with no bindings (empty ARDs)
    SQLHSTMT stmt1 = AllocExtraStmt();
    SQLHSTMT stmt2 = AllocExtraStmt();

    // Get ARD handles (both have no records — records pointer is NULL)
    SQLHDESC hArd1 = SQL_NULL_HDESC;
    SQLHDESC hArd2 = SQL_NULL_HDESC;
    SQLRETURN ret;

    ret = SQLGetStmtAttr(stmt1, SQL_ATTR_APP_ROW_DESC, &hArd1, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ASSERT_NE(hArd1, (SQLHDESC)SQL_NULL_HDESC);

    ret = SQLGetStmtAttr(stmt2, SQL_ATTR_APP_ROW_DESC, &hArd2, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ASSERT_NE(hArd2, (SQLHDESC)SQL_NULL_HDESC);

    // This previously crashed with access violation (0xC0000005)
    // because operator= tried to dereference sour.records[0] when sour.records was NULL
    ret = SQLCopyDesc(hArd1, hArd2);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCopyDesc of empty ARD should succeed, got: "
        << GetOdbcError(SQL_HANDLE_DESC, hArd2);

    // The key test is that we got here without crashing.
    // Note: The DM may report its own descriptor count for implicit descriptors,
    // so we only verify that SQLGetDescField itself succeeds.
    SQLINTEGER count = -1;
    ret = SQLGetDescField(hArd2, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_TRUE(SQL_SUCCEEDED(ret));

    SQLFreeHandle(SQL_HANDLE_STMT, stmt1);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt2);
}

TEST_F(CopyDescCrashTest, CopyEmptyToExplicitDescriptor) {
    // Allocate an explicit descriptor
    SQLHDESC hExplicit = SQL_NULL_HDESC;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hExplicit);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Get the ARD of a statement with no bindings
    SQLHDESC hArd = SQL_NULL_HDESC;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_APP_ROW_DESC, &hArd, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Copy empty ARD to explicit descriptor — must not crash
    ret = SQLCopyDesc(hArd, hExplicit);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCopyDesc from empty ARD to explicit desc failed: "
        << GetOdbcError(SQL_HANDLE_DESC, hExplicit);

    SQLFreeHandle(SQL_HANDLE_DESC, hExplicit);
}

TEST_F(CopyDescCrashTest, CopyPopulatedThenEmpty) {
    // First, populate an explicit descriptor by copying a populated ARD
    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, sizeof(val), &ind);

    SQLHDESC hArd = SQL_NULL_HDESC;
    SQLGetStmtAttr(hStmt, SQL_ATTR_APP_ROW_DESC, &hArd, 0, NULL);

    SQLHDESC hExplicit = SQL_NULL_HDESC;
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hExplicit);

    SQLRETURN ret = SQLCopyDesc(hArd, hExplicit);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Verify count = 1
    SQLINTEGER count = 0;
    SQLGetDescField(hExplicit, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_EQ(count, 1);

    // Now allocate a second explicit descriptor (which is truly empty)
    SQLHDESC hEmpty = SQL_NULL_HDESC;
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hEmpty);

    // Copy the empty explicit descriptor over the populated one — must not crash
    ret = SQLCopyDesc(hEmpty, hExplicit);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCopyDesc of empty explicit desc over populated desc failed: "
        << GetOdbcError(SQL_HANDLE_DESC, hExplicit);

    // For explicit→explicit copy (no DM interception), count should be 0
    count = 0;
    SQLGetDescField(hExplicit, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_EQ(count, 0);

    SQLFreeHandle(SQL_HANDLE_DESC, hEmpty);
    SQLFreeHandle(SQL_HANDLE_DESC, hExplicit);
}

// OC-1 Root Cause 1: SQLSetDescField(SQL_DESC_COUNT) must allocate records
TEST_F(CopyDescCrashTest, SetDescCountAllocatesRecords) {
    // Allocate an explicit descriptor
    SQLHDESC hDesc = SQL_NULL_HDESC;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hDesc);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Set SQL_DESC_COUNT to 3 — this should allocate the records array
    ret = SQLSetDescField(hDesc, 0, SQL_DESC_COUNT, (SQLPOINTER)3, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLSetDescField(SQL_DESC_COUNT, 3) failed: "
        << GetOdbcError(SQL_HANDLE_DESC, hDesc);

    // Verify the count is 3
    SQLSMALLINT count = 0;
    ret = SQLGetDescField(hDesc, 0, SQL_DESC_COUNT, &count, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, 3);

    // Now set a field on record 2 — this must NOT crash
    // (Previously, records array wasn't allocated, so this would dereference NULL)
    ret = SQLSetDescField(hDesc, 2, SQL_DESC_TYPE, (SQLPOINTER)SQL_C_SLONG, 0);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLSetDescField on record 2 after setting COUNT failed: "
        << GetOdbcError(SQL_HANDLE_DESC, hDesc);

    SQLFreeHandle(SQL_HANDLE_DESC, hDesc);
}

TEST_F(CopyDescCrashTest, SetDescCountThenCopyDesc) {
    // This is the exact odbc-crusher scenario: set SQL_DESC_COUNT then SQLCopyDesc
    SQLHDESC hSrc = SQL_NULL_HDESC;
    SQLHDESC hDst = SQL_NULL_HDESC;
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hSrc);
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hDst);

    // Set count on source without binding any columns
    SQLRETURN ret = SQLSetDescField(hSrc, 0, SQL_DESC_COUNT, (SQLPOINTER)5, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Copy source to destination — must not crash
    ret = SQLCopyDesc(hSrc, hDst);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCopyDesc after SQLSetDescField(COUNT) failed: "
        << GetOdbcError(SQL_HANDLE_DESC, hDst);

    // Verify destination has count = 5
    SQLSMALLINT count = 0;
    ret = SQLGetDescField(hDst, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, 5);

    SQLFreeHandle(SQL_HANDLE_DESC, hSrc);
    SQLFreeHandle(SQL_HANDLE_DESC, hDst);
}

TEST_F(CopyDescCrashTest, SetDescCountReduceFreesRecords) {
    // Allocate explicit descriptor and set up 3 records
    SQLHDESC hDesc = SQL_NULL_HDESC;
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hDesc);

    SQLSetDescField(hDesc, 0, SQL_DESC_COUNT, (SQLPOINTER)3, 0);

    // Set type on record 3 to verify it exists
    SQLRETURN ret = SQLSetDescField(hDesc, 3, SQL_DESC_TYPE, (SQLPOINTER)SQL_C_CHAR, 0);
    EXPECT_TRUE(SQL_SUCCEEDED(ret));

    // Reduce count to 1 — records 2 and 3 should be freed
    ret = SQLSetDescField(hDesc, 0, SQL_DESC_COUNT, (SQLPOINTER)1, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLSMALLINT count = 0;
    SQLGetDescField(hDesc, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_EQ(count, 1);

    SQLFreeHandle(SQL_HANDLE_DESC, hDesc);
}

TEST_F(CopyDescCrashTest, SetDescCountToZeroUnbindsAll) {
    // Allocate explicit descriptor and set up records
    SQLHDESC hDesc = SQL_NULL_HDESC;
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hDesc);

    SQLSetDescField(hDesc, 0, SQL_DESC_COUNT, (SQLPOINTER)2, 0);

    // Set count to 0 — should unbind all
    SQLRETURN ret = SQLSetDescField(hDesc, 0, SQL_DESC_COUNT, (SQLPOINTER)0, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLSMALLINT count = 99;
    SQLGetDescField(hDesc, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_EQ(count, 0);

    SQLFreeHandle(SQL_HANDLE_DESC, hDesc);
}

// ===== OC-2: SQL_DIAG_ROW_COUNT =====
class DiagRowCountTest : public OdbcConnectedTest {};

TEST_F(DiagRowCountTest, RowCountAfterInsert) {
    TempTable table(this, "ODBC_TEST_DIAGRC",
        "ID INTEGER NOT NULL PRIMARY KEY, NAME VARCHAR(50)");
    ReallocStmt();

    // Insert a row
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_DIAGRC VALUES (1, 'Alice')", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "INSERT failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Check SQL_DIAG_ROW_COUNT via SQLGetDiagField
    SQLLEN rowCount = -1;
    ret = SQLGetDiagField(SQL_HANDLE_STMT, hStmt, 0,
        SQL_DIAG_ROW_COUNT, &rowCount, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLGetDiagField(SQL_DIAG_ROW_COUNT) failed";
    EXPECT_EQ(rowCount, 1) << "Expected 1 row affected by INSERT";
}

TEST_F(DiagRowCountTest, RowCountAfterUpdate) {
    TempTable table(this, "ODBC_TEST_DIAGRC",
        "ID INTEGER NOT NULL PRIMARY KEY, NAME VARCHAR(50)");
    ReallocStmt();

    // Insert two rows
    SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ODBC_TEST_DIAGRC VALUES (1, 'Alice')", SQL_NTS);
    ReallocStmt();
    SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ODBC_TEST_DIAGRC VALUES (2, 'Bob')", SQL_NTS);
    ReallocStmt();
    Commit();

    // Update both rows
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"UPDATE ODBC_TEST_DIAGRC SET NAME = 'Updated'", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "UPDATE failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLLEN rowCount = -1;
    ret = SQLGetDiagField(SQL_HANDLE_STMT, hStmt, 0,
        SQL_DIAG_ROW_COUNT, &rowCount, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(rowCount, 2) << "Expected 2 rows affected by UPDATE";
}

TEST_F(DiagRowCountTest, RowCountAfterDelete) {
    TempTable table(this, "ODBC_TEST_DIAGRC",
        "ID INTEGER NOT NULL PRIMARY KEY");
    ReallocStmt();

    // Insert 3 rows
    SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ODBC_TEST_DIAGRC VALUES (1)", SQL_NTS);
    ReallocStmt();
    SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ODBC_TEST_DIAGRC VALUES (2)", SQL_NTS);
    ReallocStmt();
    SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ODBC_TEST_DIAGRC VALUES (3)", SQL_NTS);
    ReallocStmt();
    Commit();

    // Delete all
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"DELETE FROM ODBC_TEST_DIAGRC", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "DELETE failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLLEN rowCount = -1;
    ret = SQLGetDiagField(SQL_HANDLE_STMT, hStmt, 0,
        SQL_DIAG_ROW_COUNT, &rowCount, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(rowCount, 3) << "Expected 3 rows affected by DELETE";
}

TEST_F(DiagRowCountTest, RowCountAfterSelectIsMinusOne) {
    // SELECT should set SQL_DIAG_ROW_COUNT to -1 (spec says undefined for SELECTs,
    // but -1 is the conventional value used by drivers to indicate "not applicable")
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLLEN rowCount = 0;
    ret = SQLGetDiagField(SQL_HANDLE_STMT, hStmt, 0,
        SQL_DIAG_ROW_COUNT, &rowCount, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    // For SELECTs, row count is driver-defined; we set it to -1
    EXPECT_EQ(rowCount, -1) << "Expected -1 for SELECT statement";
}

// ===== OC-3: SQL_ATTR_CONNECTION_TIMEOUT =====
class ConnectionTimeoutTest : public ::testing::Test {
protected:
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hDbc = SQL_NULL_HDBC;

    void SetUp() override {
        if (GetConnectionString().empty())
            GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION not set";
    }

    void TearDown() override {
        if (hDbc != SQL_NULL_HDBC) {
            SQLDisconnect(hDbc);
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        }
        if (hEnv != SQL_NULL_HENV)
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    }

    void AllocHandles() {
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
        SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    }

    void Connect() {
        std::string connStr = GetConnectionString();
        SQLCHAR outStr[1024];
        SQLSMALLINT outLen;
        SQLRETURN ret = SQLDriverConnect(hDbc, NULL,
            (SQLCHAR*)connStr.c_str(), SQL_NTS,
            outStr, sizeof(outStr), &outLen,
            SQL_DRIVER_NOPROMPT);
        ASSERT_TRUE(SQL_SUCCEEDED(ret))
            << "Connect failed: " << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    }
};

TEST_F(ConnectionTimeoutTest, SetAndGetConnectionTimeout) {
    AllocHandles();

    // Set connection timeout before connecting
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_CONNECTION_TIMEOUT,
        (SQLPOINTER)30, SQL_IS_UINTEGER);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLSetConnectAttr(SQL_ATTR_CONNECTION_TIMEOUT) failed: "
        << GetOdbcError(SQL_HANDLE_DBC, hDbc);

    Connect();

    // Read it back
    SQLULEN timeout = 0;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_CONNECTION_TIMEOUT, &timeout, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLGetConnectAttr(SQL_ATTR_CONNECTION_TIMEOUT) failed: "
        << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    EXPECT_EQ(timeout, 30u);
}

TEST_F(ConnectionTimeoutTest, LoginTimeoutGetterWorks) {
    AllocHandles();

    // Set login timeout
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_LOGIN_TIMEOUT,
        (SQLPOINTER)15, SQL_IS_UINTEGER);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLSetConnectAttr(SQL_ATTR_LOGIN_TIMEOUT) failed";

    // Read it back — this previously fell through to HYC00 error
    SQLULEN timeout = 0;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_LOGIN_TIMEOUT, &timeout, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLGetConnectAttr(SQL_ATTR_LOGIN_TIMEOUT) failed: "
        << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    EXPECT_EQ(timeout, 15u);
}

TEST_F(ConnectionTimeoutTest, ConnectionTimeoutDefaultIsZero) {
    AllocHandles();
    Connect();

    SQLULEN timeout = 999;
    SQLRETURN ret = SQLGetConnectAttr(hDbc, SQL_ATTR_CONNECTION_TIMEOUT, &timeout, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 0u) << "Default connection timeout should be 0 (no timeout)";
}

// ===== OC-4: SQL_ATTR_ASYNC_ENABLE =====
class AsyncEnableTest : public OdbcConnectedTest {};

TEST_F(AsyncEnableTest, ConnectionLevelRejectsAsyncOn) {
    // Setting SQL_ASYNC_ENABLE_ON should fail with HYC00
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_ASYNC_ENABLE,
        (SQLPOINTER)SQL_ASYNC_ENABLE_ON, SQL_IS_UINTEGER);
    EXPECT_EQ(ret, SQL_ERROR);

    std::string state = GetSqlState(SQL_HANDLE_DBC, hDbc);
    EXPECT_EQ(state, "HYC00") << "Expected HYC00 for unsupported async enable";
}

TEST_F(AsyncEnableTest, ConnectionLevelAcceptsAsyncOff) {
    // Setting SQL_ASYNC_ENABLE_OFF should succeed (it's the default)
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_ASYNC_ENABLE,
        (SQLPOINTER)SQL_ASYNC_ENABLE_OFF, SQL_IS_UINTEGER);
    EXPECT_TRUE(SQL_SUCCEEDED(ret));
}

TEST_F(AsyncEnableTest, ConnectionLevelGetReturnsOff) {
    SQLULEN value = 999;
    SQLRETURN ret = SQLGetConnectAttr(hDbc, SQL_ATTR_ASYNC_ENABLE, &value, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(value, (SQLULEN)SQL_ASYNC_ENABLE_OFF);
}

TEST_F(AsyncEnableTest, StatementLevelRejectsAsyncOn) {
    // Setting SQL_ASYNC_ENABLE_ON on statement should fail with HYC00
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_ASYNC_ENABLE,
        (SQLPOINTER)SQL_ASYNC_ENABLE_ON, SQL_IS_UINTEGER);
    EXPECT_EQ(ret, SQL_ERROR);

    std::string state = GetSqlState(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(state, "HYC00") << "Expected HYC00 for unsupported async enable";
}

TEST_F(AsyncEnableTest, StatementLevelGetReturnsOff) {
    SQLULEN value = 999;
    SQLRETURN ret = SQLGetStmtAttr(hStmt, SQL_ATTR_ASYNC_ENABLE, &value, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(value, (SQLULEN)SQL_ASYNC_ENABLE_OFF);
}

// ===== OC-5: returnStringInfo truncation reports full length =====
class TruncationIndicatorTest : public OdbcConnectedTest {};

TEST_F(TruncationIndicatorTest, GetConnectAttrTruncationReportsFullLength) {
    // SQL_ATTR_CURRENT_CATALOG returns the database path, which is typically long
    // First, get the full length
    SQLINTEGER fullLen = 0;
    char fullBuf[1024] = {};
    SQLRETURN ret = SQLGetConnectAttr(hDbc, SQL_ATTR_CURRENT_CATALOG,
        fullBuf, sizeof(fullBuf), &fullLen);

    if (!SQL_SUCCEEDED(ret)) {
        GTEST_SKIP() << "SQL_ATTR_CURRENT_CATALOG not available";
    }

    // Skip if the catalog name is too short to trigger truncation
    if (fullLen <= 5) {
        GTEST_SKIP() << "Catalog name too short for truncation test (len=" << fullLen << ")";
    }

    // Now try with a small buffer that will trigger truncation
    char smallBuf[6] = {};  // Very small buffer
    SQLINTEGER reportedLen = 0;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_CURRENT_CATALOG,
        smallBuf, sizeof(smallBuf), &reportedLen);

    // Should return SQL_SUCCESS_WITH_INFO (truncation)
    EXPECT_EQ(ret, SQL_SUCCESS_WITH_INFO)
        << "Expected SQL_SUCCESS_WITH_INFO for truncated result";

    // The reported length should be the FULL string length, not the truncated length
    EXPECT_EQ(reportedLen, fullLen)
        << "Truncated call should report full length (" << fullLen
        << "), not truncated length";
}

TEST_F(TruncationIndicatorTest, GetInfoStringTruncationReportsFullLength) {
    // Use SQL_DBMS_NAME which is always available
    char fullBuf[256] = {};
    SQLSMALLINT fullLen = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_DBMS_NAME,
        fullBuf, sizeof(fullBuf), &fullLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ASSERT_GT(fullLen, 0) << "DBMS name should have nonzero length";

    // Now try with a buffer too small (2 bytes: 1 char + null terminator)
    char smallBuf[2] = {};
    SQLSMALLINT reportedLen = 0;
    ret = SQLGetInfo(hDbc, SQL_DBMS_NAME,
        smallBuf, sizeof(smallBuf), &reportedLen);

    // Should return SQL_SUCCESS_WITH_INFO
    EXPECT_EQ(ret, SQL_SUCCESS_WITH_INFO);

    // The reported length should be the FULL string length
    EXPECT_EQ(reportedLen, fullLen)
        << "Truncated SQLGetInfo should report full length (" << fullLen
        << "), not truncated length (" << reportedLen << ")";
}

TEST_F(TruncationIndicatorTest, GetInfoZeroBufferReportsFullLength) {
    // Call with NULL buffer and 0 length — should report length without copying
    SQLSMALLINT fullLen = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_DBMS_NAME,
        NULL, 0, &fullLen);

    // Should return SQL_SUCCESS_WITH_INFO (data available but not copied)
    EXPECT_TRUE(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
    EXPECT_GT(fullLen, 0) << "Should report the full string length even with NULL buffer";
}
