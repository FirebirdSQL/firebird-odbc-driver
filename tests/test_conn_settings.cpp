// test_conn_settings.cpp — Tests for ConnSettings (SQL on connect) feature (Task 4.6)
#include "test_helpers.h"

// ============================================================================
// ConnSettingsTest: Verify SQL is executed during connection
// ============================================================================
class ConnSettingsTest : public ::testing::Test {
protected:
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hDbc = SQL_NULL_HDBC;
    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    std::string baseConnStr;

    void SetUp() override {
        baseConnStr = GetConnectionString();
        if (baseConnStr.empty()) {
            GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION not set";
        }

        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
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

    bool Connect(const std::string& connStr) {
        SQLCHAR outBuf[1024];
        SQLSMALLINT outLen;
        SQLRETURN ret = SQLDriverConnect(hDbc, NULL,
            (SQLCHAR*)connStr.c_str(), SQL_NTS,
            outBuf, sizeof(outBuf), &outLen,
            SQL_DRIVER_NOPROMPT);
        return SQL_SUCCEEDED(ret);
    }
};

TEST_F(ConnSettingsTest, ConnSettingsExecutesSQL) {
    // Connect with ConnSettings that creates a GTT (Global Temporary Table)
    std::string connStr = baseConnStr +
        ";ConnSettings=RECREATE GLOBAL TEMPORARY TABLE CS_TEST (X INTEGER) ON COMMIT DELETE ROWS";

    ASSERT_TRUE(Connect(connStr))
        << "Connection with ConnSettings failed: "
        << GetOdbcError(SQL_HANDLE_DBC, hDbc);

    // Verify the table was created by inserting into it
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO CS_TEST (X) VALUES (42)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "INSERT into ConnSettings-created table failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Cleanup: drop the GTT
    SQLHSTMT dropStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &dropStmt);
    SQLExecDirect(dropStmt, (SQLCHAR*)"DROP TABLE CS_TEST", SQL_NTS);
    SQLFreeHandle(SQL_HANDLE_STMT, dropStmt);
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
}

TEST_F(ConnSettingsTest, EmptyConnSettingsIsIgnored) {
    // An empty ConnSettings should not cause any issues
    std::string connStr = baseConnStr + ";ConnSettings=";

    ASSERT_TRUE(Connect(connStr))
        << "Connection with empty ConnSettings failed: "
        << GetOdbcError(SQL_HANDLE_DBC, hDbc);
}

TEST_F(ConnSettingsTest, InvalidConnSettingsFailsConnection) {
    // Invalid SQL in ConnSettings should fail the connection
    std::string connStr = baseConnStr + ";ConnSettings=THIS IS NOT VALID SQL AT ALL";

    bool connected = Connect(connStr);
    // The connection should fail due to invalid SQL
    EXPECT_FALSE(connected)
        << "Expected connection to fail with invalid ConnSettings SQL";
}
