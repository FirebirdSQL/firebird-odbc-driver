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

// Convert an ASCII C-string to a null-terminated SQLWCHAR vector. Cannot use
// L"..." literals — sizeof(wchar_t) != sizeof(SQLWCHAR) on Linux.
inline std::vector<SQLWCHAR> ToSqlWchar(const char* s) {
    std::vector<SQLWCHAR> out;
    while (*s) {
        out.push_back((SQLWCHAR)(unsigned char)*s++);
    }
    out.push_back(0);
    return out;
}

// Convert a null-terminated SQLWCHAR string to a narrow std::string (low byte
// only, so ASCII round-trips correctly; non-ASCII is lossy but these helpers
// are for tests, not production data).
inline std::string FromSqlWchar(const SQLWCHAR* s) {
    std::string out;
    if (!s) return out;
    while (*s) {
        out.push_back((char)(*s & 0xFF));
        ++s;
    }
    return out;
}

// Length of a null-terminated SQLWCHAR string, in SQLWCHAR units.
inline size_t SqlWcharLen(const SQLWCHAR* s) {
    size_t n = 0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
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

// Returns the Firebird server major version number (e.g. 5 for Firebird 5.x, 6 for 6.x).
//
// The Firebird ODBC driver's SQL_DBMS_VER string is NOT the Firebird product
// version — it is constructed from the implementation / protocol version
// fields of `isc_info_version` (see IscDbc/Attachment.cpp:480) and currently
// reads e.g. "06.03.1683 WI-V Firebird 5.0" on Firebird 5.0.3.  atoi-ing that
// to get the major returns 6 for every supported server, so anything gated on
// `>= 6` (notably SKIP_ON_FIREBIRD6) silently fires on FB 3 / 4 / 5 too and
// the test is effectively disabled.
//
// Ask the server directly instead — `rdb$get_context('SYSTEM', 'ENGINE_VERSION')`
// returns a straight "5.0.3" / "6.0.0.xxxx" / "3.0.12" string, trivial to parse.
inline int GetServerMajorVersion(SQLHDBC hDbc) {
    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt))) return 0;

    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT RDB$GET_CONTEXT('SYSTEM', 'ENGINE_VERSION') FROM RDB$DATABASE",
        SQL_NTS);
    if (!SQL_SUCCEEDED(ret) || !SQL_SUCCEEDED(SQLFetch(hStmt))) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return 0;
    }

    SQLCHAR version[64] = {};
    SQLLEN ind = 0;
    SQLGetData(hStmt, 1, SQL_C_CHAR, version, sizeof(version), &ind);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return std::atoi((char*)version);
}

// Skip a test when running against Firebird 6+.
// Parameterized query handling changed in Firebird 6 and these tests need updating.
#define SKIP_ON_FIREBIRD6() \
    do { \
        if (GetServerMajorVersion(hDbc) >= 6) { \
            GTEST_SKIP() << "Broken on Firebird 6 (parameterized query incompatibility - TODO: fix)"; \
        } \
    } while (0)
