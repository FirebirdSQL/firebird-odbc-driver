// tests/test_connect_options.cpp — Connection option tests (Phase 6, ported from psqlodbc connect-test)
//
// Tests SQLConnect, SQLDriverConnect, attribute persistence, and transaction behavior.

#include "test_helpers.h"
#include <thread>
#include <chrono>

// ===== Raw connection tests (not using the fixture) =====

class ConnectOptionsTest : public ::testing::Test {
protected:
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hDbc = SQL_NULL_HDBC;
    SQLHSTMT hStmt = SQL_NULL_HSTMT;

    void SetUp() override {
        if (GetConnectionString().empty()) {
            GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION not set";
        }
    }

    void TearDown() override {
        if (hStmt != SQL_NULL_HSTMT) {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        if (hDbc != SQL_NULL_HDBC) {
            SQLDisconnect(hDbc);
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        }
        if (hEnv != SQL_NULL_HENV) {
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        }
    }

    void AllocEnvAndDbc() {
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
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

// Test basic SQLDriverConnect
TEST_F(ConnectOptionsTest, BasicDriverConnect) {
    AllocEnvAndDbc();
    Connect();
    // If we got here, connection succeeded
}

// Test that autocommit attribute persists across SQLDriverConnect
// (ported from psqlodbc test_setting_attribute_before_connect)
TEST_F(ConnectOptionsTest, AutocommitPersistsAcrossConnect) {
    AllocEnvAndDbc();

    // Disable autocommit BEFORE connecting
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLSetConnectAttr(AUTOCOMMIT_OFF) failed before connect";

    Connect();

    // Verify autocommit is still off after connect
    SQLULEN value = 0;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, &value, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLGetConnectAttr failed: " << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    EXPECT_EQ(value, (SQLULEN)SQL_AUTOCOMMIT_OFF)
        << "Autocommit should still be OFF after connect";
}

// Test that transactions work correctly with autocommit off
TEST_F(ConnectOptionsTest, RollbackUndoesInsert) {
    AllocEnvAndDbc();

    // Set autocommit OFF before connecting
    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    Connect();

    // Create temp table
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Drop if exists
    SQLExecDirect(hStmt, (SQLCHAR*)"DROP TABLE ODBC_TEST_ROLLBACK", SQL_NTS);
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"CREATE TABLE ODBC_TEST_ROLLBACK (ID INTEGER, VAL VARCHAR(50))", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "CREATE TABLE failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);

    // Insert a row
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_ROLLBACK VALUES (10000, 'should not be here')", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Rollback
    ret = SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_ROLLBACK);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Rollback failed: " << GetOdbcError(SQL_HANDLE_DBC, hDbc);

    // Verify row is NOT there
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT COUNT(*) FROM ODBC_TEST_ROLLBACK WHERE ID = 10000", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER count = -1;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, 0, &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, 0) << "Row should not exist after rollback";

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLExecDirect(hStmt, (SQLCHAR*)"DROP TABLE ODBC_TEST_ROLLBACK", SQL_NTS);
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
}

// Test that autocommit ON commits every statement automatically
TEST_F(ConnectOptionsTest, AutocommitOnCommitsEveryStatement) {
    AllocEnvAndDbc();
    Connect(); // Default: autocommit ON

    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Drop if exists
    SQLExecDirect(hStmt, (SQLCHAR*)"DROP TABLE ODBC_TEST_AUTOCOMMIT", SQL_NTS);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"CREATE TABLE ODBC_TEST_AUTOCOMMIT (ID INTEGER)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ODBC_TEST_AUTOCOMMIT VALUES (42)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Don't call SQLEndTran — autocommit should have committed already

    // Verify by reading back
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT ID FROM ODBC_TEST_AUTOCOMMIT", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 42);

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLExecDirect(hStmt, (SQLCHAR*)"DROP TABLE ODBC_TEST_AUTOCOMMIT", SQL_NTS);
}

// Test toggling autocommit on and off
TEST_F(ConnectOptionsTest, ToggleAutocommit) {
    AllocEnvAndDbc();
    Connect();

    SQLULEN value = 0;
    SQLRETURN ret;

    // Default should be ON
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, &value, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(value, (SQLULEN)SQL_AUTOCOMMIT_ON);

    // Turn OFF
    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, &value, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(value, (SQLULEN)SQL_AUTOCOMMIT_OFF);

    // Turn back ON
    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, &value, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(value, (SQLULEN)SQL_AUTOCOMMIT_ON);
}

// Test connection timeout attribute
TEST_F(ConnectOptionsTest, ConnectionTimeoutAttribute) {
    AllocEnvAndDbc();

    // Set connection timeout before connect
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_CONNECTION_TIMEOUT,
        (SQLPOINTER)30, SQL_IS_UINTEGER);
    // May or may not succeed depending on driver support, just shouldn't crash
    (void)ret;

    Connect();

    // Read it back
    SQLULEN timeout = 0;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_CONNECTION_TIMEOUT, &timeout, 0, NULL);
    if (SQL_SUCCEEDED(ret)) {
        // If supported, should return the set value or 0
        SUCCEED();
    }
}

// Test SQL_ATTR_ACCESS_MODE attribute
TEST_F(ConnectOptionsTest, AccessModeAttribute) {
    AllocEnvAndDbc();
    Connect();

    SQLULEN mode = 0;
    SQLRETURN ret = SQLGetConnectAttr(hDbc, SQL_ATTR_ACCESS_MODE, &mode, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    // Default should be read-write
    EXPECT_EQ(mode, (SQLULEN)SQL_MODE_READ_WRITE);
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

// ===== SQL_ASYNC_MODE info =====

class AsyncModeTest : public OdbcConnectedTest {};

TEST_F(AsyncModeTest, ReportsAsyncModeNone) {
    SQLUINTEGER asyncMode = 0;
    SQLSMALLINT actualLen = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_ASYNC_MODE, &asyncMode, sizeof(asyncMode), &actualLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLGetInfo(SQL_ASYNC_MODE) failed: " << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    EXPECT_EQ(asyncMode, (SQLUINTEGER)SQL_AM_NONE)
        << "SQL_ASYNC_MODE should be SQL_AM_NONE (0), got " << asyncMode;
}

// ===== SQL_ATTR_QUERY_TIMEOUT and SQLCancel tests =====

class QueryTimeoutTest : public OdbcConnectedTest {};

// Default query timeout should be 0
TEST_F(QueryTimeoutTest, DefaultTimeoutIsZero) {
    SQLULEN timeout = 999;
    SQLRETURN ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 0u) << "Default query timeout should be 0";
}

// Setting and getting timeout
TEST_F(QueryTimeoutTest, SetAndGetTimeout) {
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)5, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Failed to set query timeout: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLULEN timeout = 0;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 5u) << "Query timeout should be 5 after setting";
}

// Setting timeout back to 0 disables it
TEST_F(QueryTimeoutTest, SetTimeoutToZero) {
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)10, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)0, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLULEN timeout = 999;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 0u) << "Query timeout should be 0 after reset";
}

// SQLCancel succeeds even when nothing is executing
TEST_F(QueryTimeoutTest, CancelWhenIdleSucceeds) {
    SQLRETURN ret = SQLCancel(hStmt);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCancel on idle statement should succeed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
}

// SQLCancel from another thread interrupts a long-running query
TEST_F(QueryTimeoutTest, CancelFromAnotherThread) {
    // Use a cartesian product query that takes a long time
    const char* longQuery =
        "SELECT COUNT(*) FROM rdb$fields A "
        "CROSS JOIN rdb$fields B "
        "CROSS JOIN rdb$fields C";

    SQLHSTMT cancelStmt = hStmt;

    // Start a thread that will cancel after a short delay
    std::thread cancelThread([cancelStmt]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        SQLCancel(cancelStmt);
    });

    SQLRETURN ret = SQLExecDirect(hStmt, (SQLCHAR*)longQuery, SQL_NTS);

    cancelThread.join();

    // The query may have completed before cancel fired, or may have been cancelled.
    if (ret == SQL_ERROR) {
        std::string state = GetSqlState(SQL_HANDLE_STMT, hStmt);
        EXPECT_TRUE(state == "HY008" || state == "HY000" || state == "HYT00")
            << "Expected cancel-related SQLSTATE, got " << state;
    }
    // If SQL_SUCCESS, the query completed before cancel — that's OK too
}

// Timer-based timeout automatically cancels a long-running query
TEST_F(QueryTimeoutTest, TimerFiresOnLongQuery) {
    // Set a very short timeout (1 second)
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)1, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Execute a heavy cartesian product that should take more than 1 second
    const char* longQuery =
        "SELECT COUNT(*) FROM rdb$fields A "
        "CROSS JOIN rdb$fields B "
        "CROSS JOIN rdb$fields C "
        "CROSS JOIN rdb$fields D";

    auto start = std::chrono::steady_clock::now();
    ret = SQLExecDirect(hStmt, (SQLCHAR*)longQuery, SQL_NTS);
    auto elapsed = std::chrono::steady_clock::now() - start;

    if (ret == SQL_ERROR) {
        std::string state = GetSqlState(SQL_HANDLE_STMT, hStmt);
        EXPECT_EQ(state, "HYT00")
            << "Timer-triggered cancel should produce HYT00, got " << state;

        auto secs = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        EXPECT_LE(secs, 5) << "Should cancel within a few seconds of timeout";
    }
    // If SQL_SUCCESS, query was too fast for the timer — acceptable
}

// Timeout of 0 means no timeout — query should complete normally
TEST_F(QueryTimeoutTest, ZeroTimeoutDoesNotCancel) {
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)0, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Simple query with timeout=0 should succeed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLGetData(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    EXPECT_EQ(val, 1);
}

// ===== SQL_ATTR_RESET_CONNECTION tests =====

class ConnectionResetTest : public OdbcConnectedTest {};

// After reset, autocommit should be ON
TEST_F(ConnectionResetTest, ResetRestoresAutocommit) {
    // Turn off autocommit
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Reset connection
    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Check autocommit is back ON
    SQLUINTEGER ac = SQL_AUTOCOMMIT_OFF;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, &ac, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ac, (SQLUINTEGER)SQL_AUTOCOMMIT_ON)
        << "Autocommit should be ON after reset";
}

// After reset, transaction isolation should be default (0)
TEST_F(ConnectionResetTest, ResetRestoresTransactionIsolation) {
    // Set transaction isolation
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_TXN_ISOLATION,
                                      (SQLPOINTER)SQL_TXN_SERIALIZABLE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Reset
    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Check isolation is back to default
    SQLUINTEGER iso = SQL_TXN_SERIALIZABLE;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_TXN_ISOLATION, &iso, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(iso, 0u)
        << "Transaction isolation should be 0 (default) after reset";
}

// Uncommitted data should be rolled back on reset
TEST_F(ConnectionResetTest, ResetRollsBackPendingTransaction) {
    // Turn off autocommit so we can control transactions
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Create a temp table and insert a row WITHOUT committing
    ExecIgnoreError("DROP TABLE T11_RESET_TEST");
    // We need to commit the DROP
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    ReallocStmt();

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"CREATE TABLE T11_RESET_TEST (ID INTEGER)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "CREATE TABLE failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    ReallocStmt();

    // Insert without commit
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO T11_RESET_TEST VALUES (42)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "INSERT failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Close the cursor if any
    SQLFreeStmt(hStmt, SQL_CLOSE);

    // Reset connection — should rollback the uncommitted INSERT
    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Reset failed: " << GetOdbcError(SQL_HANDLE_DBC, hDbc);

    // Now check: the INSERT should have been rolled back
    ReallocStmt();
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT COUNT(*) FROM T11_RESET_TEST", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SELECT failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER count = -1;
    SQLLEN ind = 0;
    SQLGetData(hStmt, 1, SQL_C_SLONG, &count, 0, &ind);
    EXPECT_EQ(count, 0) << "Uncommitted INSERT should have been rolled back by reset";

    // Cleanup
    SQLFreeStmt(hStmt, SQL_CLOSE);
    ExecIgnoreError("DROP TABLE T11_RESET_TEST");
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
}

// Connection should be reusable after reset
TEST_F(ConnectionResetTest, ConnectionReusableAfterReset) {
    // Reset
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                                      (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Execute a simple query to verify the connection still works
    ReallocStmt();
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Query after reset failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLGetData(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    EXPECT_EQ(val, 1);
}

// Open cursor should be closed after reset
TEST_F(ConnectionResetTest, ResetClosesOpenCursors) {
    // Open a cursor
    SQLRETURN ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Don't fetch — leave cursor open

    // Reset connection
    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // The statement should be reusable for a new query
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT 2 FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Query after cursor-closing reset failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLGetData(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    EXPECT_EQ(val, 2);
}

// Reset should restore query timeout on child statements to 0
TEST_F(ConnectionResetTest, ResetResetsQueryTimeout) {
    // Set a non-default timeout
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)30, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Verify it was set
    SQLULEN timeout = 0;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 30u);

    // Reset connection
    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Query timeout should be back to 0
    timeout = 999;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 0u)
        << "Query timeout should be 0 after connection reset";
}
