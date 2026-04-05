// tests/test_connection_attrs.cpp — Connection attribute tests
//
// OC-3: SQL_ATTR_CONNECTION_TIMEOUT support
// OC-4: SQL_ATTR_ASYNC_ENABLE properly rejected
//
// (Migrated from test_phase7_crusher_fixes.cpp during Phase 16 deduplication.
//  OC-1 CopyDescCrashTest → test_descriptor.cpp
//  OC-2 DiagRowCountTest → test_errors.cpp
//  OC-5 TruncationIndicatorTest → test_errors.cpp)

#include "test_helpers.h"

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

    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_CONNECTION_TIMEOUT,
        (SQLPOINTER)30, SQL_IS_UINTEGER);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLSetConnectAttr(SQL_ATTR_CONNECTION_TIMEOUT) failed: "
        << GetOdbcError(SQL_HANDLE_DBC, hDbc);

    Connect();

    SQLULEN timeout = 0;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_CONNECTION_TIMEOUT, &timeout, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLGetConnectAttr(SQL_ATTR_CONNECTION_TIMEOUT) failed: "
        << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    EXPECT_EQ(timeout, 30u);
}

TEST_F(ConnectionTimeoutTest, LoginTimeoutGetterWorks) {
    AllocHandles();

    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_LOGIN_TIMEOUT,
        (SQLPOINTER)15, SQL_IS_UINTEGER);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLSetConnectAttr(SQL_ATTR_LOGIN_TIMEOUT) failed";

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
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_ASYNC_ENABLE,
        (SQLPOINTER)SQL_ASYNC_ENABLE_ON, SQL_IS_UINTEGER);
    EXPECT_EQ(ret, SQL_ERROR);

    std::string state = GetSqlState(SQL_HANDLE_DBC, hDbc);
    EXPECT_EQ(state, "HYC00") << "Expected HYC00 for unsupported async enable";
}

TEST_F(AsyncEnableTest, ConnectionLevelAcceptsAsyncOff) {
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
