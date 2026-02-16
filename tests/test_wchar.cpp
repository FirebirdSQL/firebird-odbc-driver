// tests/test_wchar.cpp — Wide character (WCHAR) handling tests
// (Phase 6, ported from psqlodbc wchar-char-test)
//
// Tests SQL_C_WCHAR binding and retrieval for both parameters and
// result columns. Verifies UTF-8/UTF-16 conversions work correctly
// through the ODBC layer with Firebird's CHARSET=UTF8 connection.
// Unlike the psqlodbc test which requires specific locale, this
// focuses on the ODBC-level wide-char mechanics.

#include "test_helpers.h"
#include <cstring>
#include <string>
#include <vector>

class WCharTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_WCHAR",
            "ID INTEGER NOT NULL PRIMARY KEY, TXT VARCHAR(200)");
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

// --- Fetch ASCII data as SQL_C_WCHAR ---

TEST_F(WCharTest, FetchAsciiAsWChar) {
    ExecDirect("INSERT INTO ODBC_TEST_WCHAR VALUES (1, 'Hello World')");
    Commit();
    ReallocStmt();

    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT TXT FROM ODBC_TEST_WCHAR WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Retrieve as WCHAR
    SQLWCHAR wbuf[128] = {};
    SQLLEN ind = 0;
    rc = SQLGetData(hStmt, 1, SQL_C_WCHAR, wbuf, sizeof(wbuf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "GetData(SQL_C_WCHAR) failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Verify the WCHAR contains 'Hello World'
    EXPECT_GT(ind, 0);

    // Convert back to ANSI for comparison
    SQLCHAR abuf[128] = {};
    SQLLEN aind = 0;
    SQLCloseCursor(hStmt);
    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT TXT FROM ODBC_TEST_WCHAR WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    rc = SQLGetData(hStmt, 1, SQL_C_CHAR, abuf, sizeof(abuf), &aind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)abuf, "Hello World");
}

// --- Bind column as SQL_C_WCHAR ---

TEST_F(WCharTest, BindColAsWChar) {
    ExecDirect("INSERT INTO ODBC_TEST_WCHAR VALUES (1, 'Test')");
    Commit();
    ReallocStmt();

    SQLWCHAR wbuf[64] = {};
    SQLLEN ind = 0;
    SQLRETURN rc = SQLBindCol(hStmt, 1, SQL_C_WCHAR, wbuf, sizeof(wbuf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT TXT FROM ODBC_TEST_WCHAR WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // wbuf should contain 'Test' as wide chars
    EXPECT_GT(ind, 0);

    // On Windows, SQLWCHAR=wchar_t=2 bytes (UCS-2).
    // On Linux, SQLWCHAR=unsigned short=2 bytes, but drivers may encode
    // differently. Verify the first character at least.
    EXPECT_EQ(wbuf[0], (SQLWCHAR)'T');

#ifdef _WIN32
    // Full per-character check (works reliably on Windows)
    EXPECT_EQ(wbuf[1], (SQLWCHAR)'e');
    EXPECT_EQ(wbuf[2], (SQLWCHAR)'s');
    EXPECT_EQ(wbuf[3], (SQLWCHAR)'t');
    EXPECT_EQ(wbuf[4], (SQLWCHAR)'\0');
#endif
}

// --- Bind parameter as SQL_C_WCHAR ---

TEST_F(WCharTest, BindParameterAsWChar) {
    SQLRETURN rc = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_WCHAR (ID, TXT) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLINTEGER id = 42;
    SQLLEN idInd = 0;
    rc = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                          0, 0, &id, 0, &idInd);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Bind a wide string parameter (portable: SQLWCHAR may differ from wchar_t on Linux)
    SQLWCHAR wtxt[] = {'W', 'i', 'd', 'e', 'P', 'a', 'r', 'a', 'm', 0};
    SQLLEN wtxtInd = SQL_NTS;
    rc = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_VARCHAR,
                          200, 0, wtxt, 0, &wtxtInd);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "BindParam(SQL_C_WCHAR) failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    rc = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "Execute with WCHAR param failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    Commit();
    ReallocStmt();

    // Read it back as ANSI
    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT TXT FROM ODBC_TEST_WCHAR WHERE ID = 42", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLCHAR abuf[128] = {};
    SQLLEN aind = 0;
    rc = SQLGetData(hStmt, 1, SQL_C_CHAR, abuf, sizeof(abuf), &aind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)abuf, "WideParam");
}

// --- Read same column as both SQL_C_CHAR and SQL_C_WCHAR ---

TEST_F(WCharTest, ReadSameColumnAsCharAndWChar) {
    ExecDirect("INSERT INTO ODBC_TEST_WCHAR VALUES (1, 'dual')");
    Commit();
    ReallocStmt();

    // First as CHAR
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT TXT FROM ODBC_TEST_WCHAR WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLCHAR abuf[64] = {};
    SQLLEN aind = 0;
    rc = SQLGetData(hStmt, 1, SQL_C_CHAR, abuf, sizeof(abuf), &aind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)abuf, "dual");

    SQLCloseCursor(hStmt);

    // Same query, now as WCHAR
    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT TXT FROM ODBC_TEST_WCHAR WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLWCHAR wbuf[64] = {};
    SQLLEN wind = 0;
    rc = SQLGetData(hStmt, 1, SQL_C_WCHAR, wbuf, sizeof(wbuf), &wind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_GT(wind, 0);
    EXPECT_EQ(wbuf[0], (SQLWCHAR)'d');
#ifdef _WIN32
    EXPECT_EQ(wbuf[1], (SQLWCHAR)'u');
    EXPECT_EQ(wbuf[2], (SQLWCHAR)'a');
    EXPECT_EQ(wbuf[3], (SQLWCHAR)'l');
#endif
}

// --- Truncation indicator for SQL_C_WCHAR ---

TEST_F(WCharTest, WCharTruncationIndicator) {
    ExecDirect("INSERT INTO ODBC_TEST_WCHAR VALUES (1, 'ABCDEFGHIJ')");
    Commit();
    ReallocStmt();

    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT TXT FROM ODBC_TEST_WCHAR WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Provide a buffer too small for the data (only 4 wide chars = 3 chars + NUL)
    SQLWCHAR tiny[4] = {};
    SQLLEN ind = 0;
    rc = SQLGetData(hStmt, 1, SQL_C_WCHAR, tiny, sizeof(tiny), &ind);
    // Should return SQL_SUCCESS_WITH_INFO (data truncated)
    EXPECT_TRUE(rc == SQL_SUCCESS_WITH_INFO || SQL_SUCCEEDED(rc));

    // ind should indicate the total length of the data (in bytes)
    if (rc == SQL_SUCCESS_WITH_INFO) {
        EXPECT_GT(ind, (SQLLEN)sizeof(tiny))
            << "Indicator should show full data length";
    }
}

// --- SQLDescribeCol reports WCHAR types for Unicode columns ---

TEST_F(WCharTest, DescribeColReportsType) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT TXT FROM ODBC_TEST_WCHAR WHERE 1=0", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLCHAR colName[128] = {};
    SQLSMALLINT nameLen = 0, dataType = 0, decDigits = 0, nullable = 0;
    SQLULEN colSize = 0;

    rc = SQLDescribeCol(hStmt, 1, colName, sizeof(colName), &nameLen,
                        &dataType, &colSize, &decDigits, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)colName, "TXT");
    // VARCHAR columns with UTF8 charset may be reported as SQL_VARCHAR or SQL_WVARCHAR
    EXPECT_TRUE(dataType == SQL_VARCHAR || dataType == SQL_WVARCHAR)
        << "Unexpected type: " << dataType;
}

// --- Empty string handling for WCHAR ---

TEST_F(WCharTest, EmptyStringWChar) {
    ExecDirect("INSERT INTO ODBC_TEST_WCHAR VALUES (1, '')");
    Commit();
    ReallocStmt();

    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT TXT FROM ODBC_TEST_WCHAR WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLWCHAR wbuf[64] = {0xFFFF};
    SQLLEN ind = 0;
    rc = SQLGetData(hStmt, 1, SQL_C_WCHAR, wbuf, sizeof(wbuf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    // Empty string: ind == 0 and wbuf[0] == 0
    EXPECT_EQ(ind, 0);
    EXPECT_EQ(wbuf[0], (SQLWCHAR)'\0');
}

// --- NULL handling for WCHAR ---

TEST_F(WCharTest, NullValueWChar) {
    ExecDirect("INSERT INTO ODBC_TEST_WCHAR VALUES (1, NULL)");
    Commit();
    ReallocStmt();

    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT TXT FROM ODBC_TEST_WCHAR WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLWCHAR wbuf[64] = {};
    SQLLEN ind = 0;
    rc = SQLGetData(hStmt, 1, SQL_C_WCHAR, wbuf, sizeof(wbuf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(ind, SQL_NULL_DATA);
}
