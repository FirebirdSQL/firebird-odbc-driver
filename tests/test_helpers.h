#pragma once

#include <gtest/gtest.h>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

// Helper to get connection string from environment
inline std::string GetConnectionString() {
    const char* connStr = std::getenv("FIREBIRD_ODBC_CONNECTION");
    if (connStr == nullptr) {
        return "";
    }
    return std::string(connStr);
}

// Skip macro for tests that need a database connection
#define REQUIRE_FIREBIRD_CONNECTION()                                    \
    do {                                                                 \
        std::string connStr = GetConnectionString();                     \
        if (connStr.empty()) {                                           \
            GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION not set";          \
        }                                                                \
    } while (0)

// Get ODBC error message from a handle
inline std::string GetOdbcError(SQLSMALLINT handleType, SQLHANDLE handle) {
    SQLCHAR sqlState[6] = {};
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH] = {};
    SQLINTEGER nativeError = 0;
    SQLSMALLINT messageLength = 0;

    SQLRETURN ret = SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeError,
                                  message, sizeof(message), &messageLength);
    if (SQL_SUCCEEDED(ret)) {
        return std::string("[") + (char*)sqlState + "] " + (char*)message;
    }
    return "(no error info)";
}

// Get SQLSTATE from a handle
inline std::string GetSqlState(SQLSMALLINT handleType, SQLHANDLE handle) {
    SQLCHAR sqlState[6] = {};
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH] = {};
    SQLINTEGER nativeError = 0;
    SQLSMALLINT messageLength = 0;

    SQLRETURN ret = SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeError,
                                  message, sizeof(message), &messageLength);
    if (SQL_SUCCEEDED(ret)) {
        return std::string((char*)sqlState);
    }
    return "";
}

// Base test fixture: ODBC environment + connection + auto-cleanup
class OdbcConnectedTest : public ::testing::Test {
public:
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hDbc = SQL_NULL_HDBC;
    SQLHSTMT hStmt = SQL_NULL_HSTMT;

    void SetUp() override {
        std::string connStr = GetConnectionString();
        if (connStr.empty()) {
            GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION not set";
        }

        SQLRETURN ret;

        ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate ENV handle";

        ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to set ODBC version";

        ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate DBC handle";

        SQLCHAR outConnStr[1024];
        SQLSMALLINT outConnStrLen;
        ret = SQLDriverConnect(hDbc, NULL,
                               (SQLCHAR*)connStr.c_str(), SQL_NTS,
                               outConnStr, sizeof(outConnStr), &outConnStrLen,
                               SQL_DRIVER_NOPROMPT);
        ASSERT_TRUE(SQL_SUCCEEDED(ret))
            << "Failed to connect: " << GetOdbcError(SQL_HANDLE_DBC, hDbc);

        ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate STMT handle";
    }

    void TearDown() override {
        if (hStmt != SQL_NULL_HSTMT) {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            hStmt = SQL_NULL_HSTMT;
        }
        if (hDbc != SQL_NULL_HDBC) {
            SQLDisconnect(hDbc);
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            hDbc = SQL_NULL_HDBC;
        }
        if (hEnv != SQL_NULL_HENV) {
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
            hEnv = SQL_NULL_HENV;
        }
    }

    // Allocate a fresh statement handle (frees the previous one)
    void ReallocStmt() {
        if (hStmt != SQL_NULL_HSTMT) {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            hStmt = SQL_NULL_HSTMT;
        }
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate statement";
    }

    // Allocate a second statement handle on the same connection
    SQLHSTMT AllocExtraStmt() {
        SQLHSTMT stmt = SQL_NULL_HSTMT;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &stmt);
        EXPECT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate extra statement";
        return stmt;
    }

    // Execute SQL, ignoring errors (for DROP TABLE IF EXISTS patterns)
    void ExecIgnoreError(const char* sql) {
        SQLHSTMT stmt = SQL_NULL_HSTMT;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &stmt);
        SQLExecDirect(stmt, (SQLCHAR*)sql, SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

    // Execute SQL and assert success
    void ExecDirect(const char* sql) {
        SQLRETURN ret = SQLExecDirect(hStmt, (SQLCHAR*)sql, SQL_NTS);
        ASSERT_TRUE(SQL_SUCCEEDED(ret))
            << "SQL failed: " << sql << "\n"
            << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    }

    // Commit the current transaction
    void Commit() {
        SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Commit failed";
    }

    // Rollback the current transaction
    void Rollback() {
        SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_ROLLBACK);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Rollback failed";
    }
};

// RAII guard to create + drop a temporary table 
class TempTable {
public:
    TempTable(OdbcConnectedTest* test, const char* name, const char* columns)
        : test_(test), name_(name) {
        // Drop if exists (may fail, that's fine)
        std::string dropSql = "DROP TABLE " + name_;
        test_->ExecIgnoreError(dropSql.c_str());
        test_->Commit();
        test_->ReallocStmt();

        // Create
        std::string createSql = "CREATE TABLE " + name_ + " (" + columns + ")";
        test_->ExecDirect(createSql.c_str());
        test_->Commit();
        test_->ReallocStmt();
    }

    ~TempTable() {
        // Best-effort cleanup
        std::string dropSql = "DROP TABLE " + name_;
        test_->ExecIgnoreError(dropSql.c_str());
        // Commit the drop (ignore errors)
        SQLEndTran(SQL_HANDLE_DBC, test_->hDbc, SQL_COMMIT);
    }

private:
    OdbcConnectedTest* test_;
    std::string name_;
};
