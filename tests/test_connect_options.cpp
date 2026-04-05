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

// ===== Concurrent connection tests =====

class ConcurrentConnectionTest : public ::testing::Test {
protected:
    struct OdbcConn {
        SQLHENV hEnv = SQL_NULL_HENV;
        SQLHDBC hDbc = SQL_NULL_HDBC;
        SQLHSTMT hStmt = SQL_NULL_HSTMT;

        bool connect() {
            std::string connStr = GetConnectionString();
            SQLRETURN ret;
            ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
            if (!SQL_SUCCEEDED(ret)) return false;
            SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
            ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
            if (!SQL_SUCCEEDED(ret)) return false;
            SQLCHAR outStr[1024];
            SQLSMALLINT outLen;
            ret = SQLDriverConnect(hDbc, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS,
                outStr, sizeof(outStr), &outLen, SQL_DRIVER_NOPROMPT);
            if (!SQL_SUCCEEDED(ret)) return false;
            ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
            return SQL_SUCCEEDED(ret);
        }

        void disconnect() {
            if (hStmt != SQL_NULL_HSTMT) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            if (hDbc != SQL_NULL_HDBC) { SQLDisconnect(hDbc); SQLFreeHandle(SQL_HANDLE_DBC, hDbc); }
            if (hEnv != SQL_NULL_HENV) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        }
    };

    void SetUp() override {
        if (GetConnectionString().empty())
            GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION not set";
    }
};

TEST_F(ConcurrentConnectionTest, TwoIndependentConnections) {
    OdbcConn conn1, conn2;
    ASSERT_TRUE(conn1.connect()) << "First connection failed";
    ASSERT_TRUE(conn2.connect()) << "Second connection failed";

    // Execute queries on both connections
    SQLRETURN ret1 = SQLExecDirect(conn1.hStmt,
        (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
    SQLRETURN ret2 = SQLExecDirect(conn2.hStmt,
        (SQLCHAR*)"SELECT 2 FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret1));
    ASSERT_TRUE(SQL_SUCCEEDED(ret2));

    SQLINTEGER val1 = 0, val2 = 0;
    SQLLEN ind = 0;
    SQLFetch(conn1.hStmt);
    SQLGetData(conn1.hStmt, 1, SQL_C_SLONG, &val1, 0, &ind);
    SQLFetch(conn2.hStmt);
    SQLGetData(conn2.hStmt, 1, SQL_C_SLONG, &val2, 0, &ind);

    EXPECT_EQ(val1, 1);
    EXPECT_EQ(val2, 2);

    conn2.disconnect();
    conn1.disconnect();
}

TEST_F(ConcurrentConnectionTest, ConnectionIsolation) {
    OdbcConn conn1, conn2;
    ASSERT_TRUE(conn1.connect());
    ASSERT_TRUE(conn2.connect());

    // Turn off autocommit on conn1
    SQLSetConnectAttr(conn1.hDbc, SQL_ATTR_AUTOCOMMIT,
        (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);

    // Create table on conn1 (committed by auto-commit being off, need explicit commit)
    SQLExecDirect(conn1.hStmt, (SQLCHAR*)"DROP TABLE T_CONCURRENT_TEST", SQL_NTS);
    SQLEndTran(SQL_HANDLE_DBC, conn1.hDbc, SQL_COMMIT);
    SQLFreeStmt(conn1.hStmt, SQL_CLOSE);

    SQLRETURN ret = SQLExecDirect(conn1.hStmt,
        (SQLCHAR*)"CREATE TABLE T_CONCURRENT_TEST (ID INTEGER)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << GetOdbcError(SQL_HANDLE_STMT, conn1.hStmt);
    SQLEndTran(SQL_HANDLE_DBC, conn1.hDbc, SQL_COMMIT);
    SQLFreeHandle(SQL_HANDLE_STMT, conn1.hStmt);

    // Insert uncommitted row on conn1
    SQLAllocHandle(SQL_HANDLE_STMT, conn1.hDbc, &conn1.hStmt);
    ret = SQLExecDirect(conn1.hStmt,
        (SQLCHAR*)"INSERT INTO T_CONCURRENT_TEST VALUES (42)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    // Do NOT commit

    // conn2 should not see the uncommitted row
    ret = SQLExecDirect(conn2.hStmt,
        (SQLCHAR*)"SELECT COUNT(*) FROM T_CONCURRENT_TEST", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << GetOdbcError(SQL_HANDLE_STMT, conn2.hStmt);
    SQLFetch(conn2.hStmt);
    SQLINTEGER count = -1;
    SQLLEN ind = 0;
    SQLGetData(conn2.hStmt, 1, SQL_C_SLONG, &count, 0, &ind);
    EXPECT_EQ(count, 0) << "Uncommitted row should not be visible to other connections";

    // Cleanup
    SQLEndTran(SQL_HANDLE_DBC, conn1.hDbc, SQL_ROLLBACK);
    SQLFreeHandle(SQL_HANDLE_STMT, conn1.hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, conn1.hDbc, &conn1.hStmt);
    SQLExecDirect(conn1.hStmt, (SQLCHAR*)"DROP TABLE T_CONCURRENT_TEST", SQL_NTS);
    SQLEndTran(SQL_HANDLE_DBC, conn1.hDbc, SQL_COMMIT);

    conn2.disconnect();
    conn1.disconnect();
}
