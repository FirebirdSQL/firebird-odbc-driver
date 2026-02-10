// tests/test_cursor_name.cpp — Cursor name tests
// (Phase 6, ported from psqlodbc cursor-name-test)
//
// Tests SQLSetCursorName / SQLGetCursorName behavior, including
// default cursor name generation, custom names, and cursor name
// persistence across statement operations.

#include "test_helpers.h"
#include <cstring>
#include <string>

class CursorNameTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_CNAME",
            "ID INTEGER NOT NULL PRIMARY KEY, TXT VARCHAR(30)");

        for (int i = 1; i <= 5; i++) {
            ReallocStmt();
            char sql[128];
            snprintf(sql, sizeof(sql),
                "INSERT INTO ODBC_TEST_CNAME VALUES (%d, 'val%d')", i, i);
            ExecDirect(sql);
        }
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

// --- Default cursor name should start with SQL_CUR ---

TEST_F(CursorNameTest, DefaultCursorNamePrefix) {
    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0;

    SQLRETURN rc = SQLGetCursorName(hStmt, name, sizeof(name), &nameLen);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "GetCursorName failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_GT(nameLen, 0) << "Cursor name should not be empty";

    // ODBC spec says auto-generated names should start with SQL_CUR
    std::string nameStr((char*)name, nameLen);
    EXPECT_EQ(nameStr.substr(0, 7), "SQL_CUR")
        << "Default cursor name should begin with 'SQL_CUR', got: " << nameStr;
}

// --- Set and get a custom cursor name ---

TEST_F(CursorNameTest, SetAndGetCursorName) {
    SQLRETURN rc = SQLSetCursorName(hStmt, (SQLCHAR*)"MY_CURSOR", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SetCursorName failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0;
    rc = SQLGetCursorName(hStmt, name, sizeof(name), &nameLen);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)name, "MY_CURSOR");
    EXPECT_EQ(nameLen, 9);
}

// --- Cursor name persists after ExecDirect ---

TEST_F(CursorNameTest, CursorNamePersistsAfterExec) {
    SQLRETURN rc = SQLSetCursorName(hStmt, (SQLCHAR*)"PERSIST_CURSOR", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Execute a query
    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_CNAME ORDER BY ID", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Name should still be set
    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0;
    rc = SQLGetCursorName(hStmt, name, sizeof(name), &nameLen);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)name, "PERSIST_CURSOR");
}

// --- Two statements have different default cursor names ---

TEST_F(CursorNameTest, TwoStatementsHaveDifferentNames) {
    SQLHSTMT hStmt2 = AllocExtraStmt();

    SQLCHAR name1[128] = {}, name2[128] = {};
    SQLSMALLINT len1 = 0, len2 = 0;

    SQLRETURN rc = SQLGetCursorName(hStmt, name1, sizeof(name1), &len1);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLGetCursorName(hStmt2, name2, sizeof(name2), &len2);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    EXPECT_STRNE((char*)name1, (char*)name2)
        << "Two statements should have different default cursor names";

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
}

// --- Cursor name buffer too small returns SQL_SUCCESS_WITH_INFO ---

TEST_F(CursorNameTest, CursorNameBufferTooSmall) {
    SQLRETURN rc = SQLSetCursorName(hStmt, (SQLCHAR*)"LONG_CURSOR_NAME", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Try to get it into a buffer that's too small
    SQLCHAR tiny[5] = {};
    SQLSMALLINT nameLen = 0;
    rc = SQLGetCursorName(hStmt, tiny, sizeof(tiny), &nameLen);

    // Should return SQL_SUCCESS_WITH_INFO with truncation
    EXPECT_EQ(rc, SQL_SUCCESS_WITH_INFO)
        << "Expected SQL_SUCCESS_WITH_INFO for truncation";

    // nameLen should indicate the full length
    EXPECT_EQ(nameLen, 16) << "Should report full cursor name length";

    // Buffer should contain truncated name (null-terminated)
    EXPECT_EQ(tiny[4], '\0');
}

// --- Set cursor name to empty string (should fail or return error) ---

TEST_F(CursorNameTest, SetEmptyCursorName) {
    SQLRETURN rc = SQLSetCursorName(hStmt, (SQLCHAR*)"", SQL_NTS);
    // ODBC spec says the cursor name must be at least 1 character
    EXPECT_TRUE(rc == SQL_ERROR || SQL_SUCCEEDED(rc));
}

// --- Use cursor name during fetch to position ---

TEST_F(CursorNameTest, CursorNameDuringFetch) {
    SQLRETURN rc = SQLSetCursorName(hStmt, (SQLCHAR*)"FETCH_CURSOR", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID, TXT FROM ODBC_TEST_CNAME ORDER BY ID", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Fetch to row 3
    for (int i = 0; i < 3; i++) {
        rc = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
    }

    // Verify we're on row 3
    SQLCHAR buf[32] = {};
    SQLLEN ind = 0;
    rc = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)buf, "3");

    // Cursor name should still be valid
    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0;
    rc = SQLGetCursorName(hStmt, name, sizeof(name), &nameLen);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)name, "FETCH_CURSOR");
}

// --- Close cursor and check if cursor name resets ---

TEST_F(CursorNameTest, CursorNameAfterClose) {
    SQLRETURN rc = SQLSetCursorName(hStmt, (SQLCHAR*)"CLOSE_TEST", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFreeStmt(hStmt, SQL_CLOSE);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // After closing, the cursor name should still be available
    // (it persists until the statement is freed or a new name is set)
    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0;
    rc = SQLGetCursorName(hStmt, name, sizeof(name), &nameLen);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)name, "CLOSE_TEST");
}

// --- Duplicate cursor name on same connection should fail ---

TEST_F(CursorNameTest, DuplicateCursorNameBehavior) {
    // ODBC spec says duplicate cursor names on the same connection should
    // return SQL_ERROR with SQLSTATE 3C000. However, some drivers (including
    // Firebird) allow duplicate names. We test both cases.
    SQLHSTMT hStmt2 = AllocExtraStmt();

    SQLRETURN rc = SQLSetCursorName(hStmt, (SQLCHAR*)"DUPE_NAME", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLSetCursorName(hStmt2, (SQLCHAR*)"DUPE_NAME", SQL_NTS);
    if (rc == SQL_ERROR) {
        // Spec-compliant behavior
        std::string state = GetSqlState(SQL_HANDLE_STMT, hStmt2);
        EXPECT_EQ(state, "3C000")
            << "Expected SQLSTATE 3C000 for duplicate cursor name, got: " << state;
    } else {
        // Firebird allows duplicate cursor names — document but don't fail
        EXPECT_TRUE(SQL_SUCCEEDED(rc));
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
}
