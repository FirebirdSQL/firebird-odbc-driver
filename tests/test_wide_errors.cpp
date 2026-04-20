// tests/test_wide_errors.cpp — Widechar ODBC call tests
//
// Drives the fix for the Linux widechar conversion bugs in MainUnicode.cpp.
// Covers both ConvertingString constructor scenarios:
//
//   (a) Output path used by SQLGetDiagRecW / SQLErrorW
//       ConvertingString(length, sqlState) -> exercised by
//       GetDiagRecW_* and ErrorW_* tests below.
//
//   (b) Input path used by SQLExecDirectW / SQLPrepareW
//       ConvertingString(connection, wcString, length) +
//       convUnicodeToString -> exercised by ExecDirectW_* /
//       PrepareW_* tests below.
//
// On Linux the current driver mis-handles both paths — the widechar
// read-back is truncated to one SQLWCHAR, and a small output buffer can
// smash the caller's stack. These tests are therefore SKIP'd on Linux
// with a pointer to the follow-up Unicode-fix PR. Remove the skips once
// that PR lands.

#include "test_helpers.h"
#include <cstring>
#include <string>
#include <vector>

#ifndef _WIN32
// Shared skip body — all tests in this file depend on the same Linux
// widechar bug. Keep the message pointing at issue #287 Tier 1b so
// grepping finds every affected test at once.
#define SKIP_ON_LINUX_WIDECHAR()                                           \
    do {                                                                   \
        GTEST_SKIP() << "Widechar conversion paths broken on Linux —  "    \
                        "see issue #287 Tier 1b. Un-skip once the "        \
                        "Unicode-fix PR lands.";                           \
    } while (0)
#else
#define SKIP_ON_LINUX_WIDECHAR() do {} while (0)
#endif

class WideErrorsTest : public OdbcConnectedTest {};

// ============================================================================
// (a) Output path — SQLGetDiagRecW on an error
// ============================================================================

// Force a parse error, then read the SQLSTATE back via the widechar variant
// with a tight 12-byte (6-SQLWCHAR) output buffer. This is the exact shape of
// the ConvertingString(12, sqlState) construction inside SQLGetDiagRecW.
TEST_F(WideErrorsTest, GetDiagRecW_SqlState) {
    SKIP_ON_LINUX_WIDECHAR();

    // Trigger an error (column doesn't exist)
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT doesnotexist FROM RDB$DATABASE", SQL_NTS);
    ASSERT_FALSE(SQL_SUCCEEDED(ret));

    SQLWCHAR sqlState[6] = {};   // 12 bytes, fits "HY000\0"
    SQLINTEGER nativeError = 0;
    SQLWCHAR messageBuf[SQL_MAX_MESSAGE_LENGTH] = {};
    SQLSMALLINT messageLen = 0;

    ret = SQLGetDiagRecW(SQL_HANDLE_STMT, hStmt, 1,
                         sqlState, &nativeError,
                         messageBuf, SQL_MAX_MESSAGE_LENGTH, &messageLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLGetDiagRecW failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // SQLSTATE is exactly 5 characters.
    EXPECT_EQ(SqlWcharLen(sqlState), 5u)
        << "State decoded as: '" << FromSqlWchar(sqlState) << "'";

    // Firebird returns 42S22 (column not found) or 42000 or HY000.
    std::string state = FromSqlWchar(sqlState);
    EXPECT_TRUE(state == "42S22" || state == "42000" || state == "HY000")
        << "Unexpected SQLSTATE: '" << state << "'";
}

// Same as above but checks the message buffer, which uses the other
// ConvertingString size (bufferLength can be large). This exercises the
// widechar path with a reasonable output buffer.
TEST_F(WideErrorsTest, GetDiagRecW_Message) {
    SKIP_ON_LINUX_WIDECHAR();

    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT doesnotexist FROM RDB$DATABASE", SQL_NTS);
    ASSERT_FALSE(SQL_SUCCEEDED(ret));

    SQLWCHAR sqlState[6] = {};
    SQLINTEGER nativeError = 0;
    SQLWCHAR messageBuf[512] = {};
    SQLSMALLINT messageLen = 0;

    ret = SQLGetDiagRecW(SQL_HANDLE_STMT, hStmt, 1,
                         sqlState, &nativeError,
                         messageBuf, 512, &messageLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Message must be non-empty and readable as ASCII.
    std::string msg = FromSqlWchar(messageBuf);
    EXPECT_FALSE(msg.empty()) << "Error message was empty";

    // And the driver must report the length correctly (in SQLWCHAR units
    // according to the spec).
    EXPECT_GT(messageLen, 0);
}

// Deliberately tight output buffer — the driver must not overrun it.
// With sqlState sized at 6 SQLWCHARs (12 bytes) this is exactly the shape
// that originally triggered the heap-buffer-overflow / stack-smash pair.
TEST_F(WideErrorsTest, GetDiagRecW_SmallBuffer) {
    SKIP_ON_LINUX_WIDECHAR();

    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT doesnotexist FROM RDB$DATABASE", SQL_NTS);
    ASSERT_FALSE(SQL_SUCCEEDED(ret));

    // Guard bytes either side; any write outside sqlState[] will trip them.
    SQLWCHAR guardBefore[4];
    SQLWCHAR sqlState[6] = {};
    SQLWCHAR guardAfter[4];
    for (int i = 0; i < 4; ++i) {
        guardBefore[i] = (SQLWCHAR)0xBEEF;
        guardAfter[i]  = (SQLWCHAR)0xBEEF;
    }

    SQLINTEGER nativeError = 0;
    SQLWCHAR messageBuf[16] = {};   // intentionally tiny
    SQLSMALLINT messageLen = 0;

    ret = SQLGetDiagRecW(SQL_HANDLE_STMT, hStmt, 1,
                         sqlState, &nativeError,
                         messageBuf, 16, &messageLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret) || ret == SQL_SUCCESS_WITH_INFO);

    // Guards must be untouched.
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(guardBefore[i], (SQLWCHAR)0xBEEF) << "guardBefore[" << i << "]";
        EXPECT_EQ(guardAfter[i],  (SQLWCHAR)0xBEEF) << "guardAfter["  << i << "]";
    }

    // State still decodes to 5 characters.
    EXPECT_EQ(SqlWcharLen(sqlState), 5u)
        << "State decoded as: '" << FromSqlWchar(sqlState) << "'";
}

// Legacy ODBC 2.x SQLErrorW — same underlying ConvertingString(12, sqlState)
// shape. Kept so the fix also covers that call site.
TEST_F(WideErrorsTest, ErrorW_SqlState) {
    SKIP_ON_LINUX_WIDECHAR();

    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT doesnotexist FROM RDB$DATABASE", SQL_NTS);
    ASSERT_FALSE(SQL_SUCCEEDED(ret));

    SQLWCHAR sqlState[6] = {};
    SQLINTEGER nativeError = 0;
    SQLWCHAR messageBuf[512] = {};
    SQLSMALLINT messageLen = 0;

    ret = SQLErrorW(hEnv, hDbc, hStmt,
                    sqlState, &nativeError,
                    messageBuf, 512, &messageLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    EXPECT_EQ(SqlWcharLen(sqlState), 5u)
        << "State decoded as: '" << FromSqlWchar(sqlState) << "'";
}

// ============================================================================
// (b) Input path — SQLExecDirectW / SQLPrepareW
// ============================================================================

class WideExecTest : public OdbcConnectedTest {};

// Pass an ASCII query as SQLWCHAR with SQL_NTS — exercises the convUnicodeToString
// path's sqlwcharLen-equivalent branch (currently wcslen on a SQLWCHAR pointer,
// which is wrong on Linux).
TEST_F(WideExecTest, ExecDirectW_Ascii_Nts) {
    SKIP_ON_LINUX_WIDECHAR();

    auto query = ToSqlWchar("SELECT 1 FROM RDB$DATABASE");
    SQLRETURN ret = SQLExecDirectW(hStmt, query.data(), SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLExecDirectW failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 1);
}

// Same but with an explicit SQLWCHAR-unit length — exercises the other branch
// of convUnicodeToString where we temporarily NUL-terminate input.
TEST_F(WideExecTest, ExecDirectW_Ascii_ExplicitLength) {
    SKIP_ON_LINUX_WIDECHAR();

    auto query = ToSqlWchar("SELECT 1 FROM RDB$DATABASE");
    // Length is in characters (SQLWCHAR units), per the spec.
    SQLINTEGER len = (SQLINTEGER)(query.size() - 1);  // exclude trailing NUL

    SQLRETURN ret = SQLExecDirectW(hStmt, query.data(), len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLExecDirectW(len=" << len << ") failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 1);
}

// SQLPrepareW twin — same input path as ExecDirectW.
TEST_F(WideExecTest, PrepareW_Ascii_Nts) {
    SKIP_ON_LINUX_WIDECHAR();

    auto query = ToSqlWchar("SELECT 1 FROM RDB$DATABASE");
    SQLRETURN ret = SQLPrepareW(hStmt, query.data(), SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLPrepareW failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 1);
}

// A query long enough that the narrow byteString allocation has to grow.
// If the length accounting is wrong in SQLWCHAR vs wchar_t units, this is
// where truncation/overflow bugs surface.
TEST_F(WideExecTest, ExecDirectW_Ascii_LongQuery) {
    SKIP_ON_LINUX_WIDECHAR();

    // 200+ byte query so the byteString buffer is a meaningful size.
    std::string q = "SELECT ";
    for (int i = 0; i < 40; ++i) q += "1+";
    q += "0 FROM RDB$DATABASE";

    auto query = ToSqlWchar(q.c_str());
    SQLRETURN ret = SQLExecDirectW(hStmt, query.data(), SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLExecDirectW(long) failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
}
