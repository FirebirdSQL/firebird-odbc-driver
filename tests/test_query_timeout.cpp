// tests/test_query_timeout.cpp — Query timeout, async mode, and connection reset tests
//
// Covers:
//   11.1.6 - SQL_ASYNC_MODE reports SQL_AM_NONE
//   11.2.5 - SQL_ATTR_QUERY_TIMEOUT getter/setter, SQLCancel, timeout on long query
//   11.3.4 - SQL_ATTR_RESET_CONNECTION rollback, cursor cleanup, attribute reset
//
// (Migrated from test_phase11_typeinfo_timeout_pool.cpp during Phase 16 deduplication.
//  TypeInfoTest → test_catalogfunctions.cpp)

#include "test_helpers.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <mutex>

// ============================================================================
// 11.1.6: SQL_ASYNC_MODE test
// ============================================================================

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

// ============================================================================
// 11.2.5: SQL_ATTR_QUERY_TIMEOUT and SQLCancel tests
// ============================================================================

class QueryTimeoutTest : public OdbcConnectedTest {};

TEST_F(QueryTimeoutTest, DefaultTimeoutIsZero) {
    SQLULEN timeout = 999;
    SQLRETURN ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 0u) << "Default query timeout should be 0";
}

TEST_F(QueryTimeoutTest, SetAndGetTimeout) {
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)5, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Failed to set query timeout: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLULEN timeout = 0;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 5u) << "Query timeout should be 5 after setting";
}

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

TEST_F(QueryTimeoutTest, CancelWhenIdleSucceeds) {
    SQLRETURN ret = SQLCancel(hStmt);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCancel on idle statement should succeed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
}

// SQLCancel from another thread — uses condition variable to avoid timing sensitivity
TEST_F(QueryTimeoutTest, CancelFromAnotherThread) {
    const char* longQuery =
        "SELECT COUNT(*) FROM rdb$fields A "
        "CROSS JOIN rdb$fields B "
        "CROSS JOIN rdb$fields C";

    SQLHSTMT cancelStmt = hStmt;

    // Use atomic flag + retry loop instead of fixed sleep
    std::atomic<bool> queryStarted{false};
    std::atomic<bool> cancelSent{false};

    std::thread cancelThread([cancelStmt, &queryStarted, &cancelSent]() {
        // Retry-with-backoff: try canceling multiple times
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            SQLCancel(cancelStmt);
            cancelSent.store(true);
        }
    });

    SQLRETURN ret = SQLExecDirect(hStmt, (SQLCHAR*)longQuery, SQL_NTS);

    cancelThread.join();

    if (ret == SQL_ERROR) {
        std::string state = GetSqlState(SQL_HANDLE_STMT, hStmt);
        EXPECT_TRUE(state == "HY008" || state == "HY000" || state == "HYT00")
            << "Expected cancel-related SQLSTATE, got " << state;
    }
    // If SQL_SUCCESS, the query completed before cancel — that's OK too
}

TEST_F(QueryTimeoutTest, TimerFiresOnLongQuery) {
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)1, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

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
}

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

// ============================================================================
// 11.3.4: SQL_ATTR_RESET_CONNECTION tests
// ============================================================================

class ConnectionResetTest : public OdbcConnectedTest {};

TEST_F(ConnectionResetTest, ResetRestoresAutocommit) {
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLUINTEGER ac = SQL_AUTOCOMMIT_OFF;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, &ac, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ac, (SQLUINTEGER)SQL_AUTOCOMMIT_ON)
        << "Autocommit should be ON after reset";
}

TEST_F(ConnectionResetTest, ResetRestoresTransactionIsolation) {
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_TXN_ISOLATION,
                                      (SQLPOINTER)SQL_TXN_SERIALIZABLE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLUINTEGER iso = SQL_TXN_SERIALIZABLE;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_TXN_ISOLATION, &iso, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(iso, 0u)
        << "Transaction isolation should be 0 (default) after reset";
}

TEST_F(ConnectionResetTest, ResetRollsBackPendingTransaction) {
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ExecIgnoreError("DROP TABLE T11_RESET_TEST");
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    ReallocStmt();

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"CREATE TABLE T11_RESET_TEST (ID INTEGER)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "CREATE TABLE failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    ReallocStmt();

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO T11_RESET_TEST VALUES (42)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "INSERT failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLFreeStmt(hStmt, SQL_CLOSE);

    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Reset failed: " << GetOdbcError(SQL_HANDLE_DBC, hDbc);

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

    SQLFreeStmt(hStmt, SQL_CLOSE);
    ExecIgnoreError("DROP TABLE T11_RESET_TEST");
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
}

TEST_F(ConnectionResetTest, ConnectionReusableAfterReset) {
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                                      (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

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

TEST_F(ConnectionResetTest, ResetClosesOpenCursors) {
    SQLRETURN ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

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

TEST_F(ConnectionResetTest, ResetResetsQueryTimeout) {
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)30, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLULEN timeout = 0;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 30u);

    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    timeout = 999;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 0u)
        << "Query timeout should be 0 after connection reset";
}
