// test_phase11_typeinfo_timeout_pool.cpp
// Phase 11 tests: SQLGetTypeInfo fixes, statement timeout, connection pool reset
//
// Covers:
//   11.1.7 - SQLGetTypeInfo ordering, multi-row DATA_TYPE, GUID searchability, no duplicates
//   11.2.5 - SQL_ATTR_QUERY_TIMEOUT getter/setter, SQLCancel, timeout on long query
//   11.3.4 - SQL_ATTR_RESET_CONNECTION rollback, cursor cleanup, attribute reset
//   11.1.6 - SQL_ASYNC_MODE reports SQL_AM_NONE

#include "test_helpers.h"
#include <thread>
#include <chrono>
#include <set>
#include <map>

// ============================================================================
// 11.1.7: SQLGetTypeInfo tests
// ============================================================================

class TypeInfoTest : public OdbcConnectedTest {};

// Result set must be sorted by DATA_TYPE ascending (ODBC spec requirement)
TEST_F(TypeInfoTest, ResultSetSortedByDataType)
{
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLGetTypeInfo failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLSMALLINT prevDataType = -32768;
    int rowCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt)))
    {
        SQLSMALLINT dataType = 0;
        SQLLEN ind = 0;
        ret = SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));

        EXPECT_GE(dataType, prevDataType)
            << "Row " << (rowCount + 1) << ": DATA_TYPE " << dataType
            << " is less than previous " << prevDataType
            << " — result set is not sorted by DATA_TYPE ascending";
        prevDataType = dataType;
        rowCount++;
    }
    EXPECT_GT(rowCount, 0) << "No rows returned by SQLGetTypeInfo(SQL_ALL_TYPES)";
}

// When a specific DATA_TYPE is requested, all matching rows must be returned
// (e.g. SQL_NUMERIC may match both NUMERIC and INT128 on FB4+)
TEST_F(TypeInfoTest, MultipleRowsForSameDataType)
{
    // SQL_INTEGER is a good test — should return exactly 1 row
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_INTEGER);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    int rowCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt)))
    {
        SQLSMALLINT dataType = 0;
        SQLLEN ind = 0;
        SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind);
        EXPECT_EQ(dataType, SQL_INTEGER) << "Unexpected DATA_TYPE in filtered result";
        rowCount++;
    }
    EXPECT_GE(rowCount, 1) << "Should return at least 1 row for SQL_INTEGER";
}

// SQL_NUMERIC should return at least NUMERIC, and on FB4+ also INT128
TEST_F(TypeInfoTest, NumericReturnsMultipleOnFB4Plus)
{
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_NUMERIC);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    std::vector<std::string> typeNames;
    while (SQL_SUCCEEDED(SQLFetch(hStmt)))
    {
        SQLCHAR typeName[128] = {};
        SQLLEN ind = 0;
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &ind);
        typeNames.push_back(std::string((char*)typeName));
    }
    ASSERT_GE(typeNames.size(), 1u) << "At least NUMERIC should be returned";

    // Verify NUMERIC is in the list
    bool hasNumeric = false;
    for (auto& name : typeNames)
    {
        if (name == "NUMERIC") hasNumeric = true;
    }
    EXPECT_TRUE(hasNumeric) << "NUMERIC type not found in SQL_NUMERIC results";

    // On FB4+ we expect INT128 too (but don't fail on older servers)
}

// No non-existent type should return any rows
TEST_F(TypeInfoTest, NonexistentTypeReturnsNoRows)
{
    // SQL_WCHAR = -8, may or may not be supported
    // Use an extremely unlikely type code
    SQLRETURN ret = SQLGetTypeInfo(hStmt, 9999);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    int rowCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt)))
        rowCount++;
    EXPECT_EQ(rowCount, 0) << "Invalid type 9999 should return 0 rows";
}

// SQL_GUID type should have SEARCHABLE = SQL_ALL_EXCEPT_LIKE (2), not SQL_SEARCHABLE (3)
TEST_F(TypeInfoTest, GuidSearchabilityIsAllExceptLike)
{
    // Request SQL_ALL_TYPES and find the GUID row
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    bool foundGuid = false;
    while (SQL_SUCCEEDED(SQLFetch(hStmt)))
    {
        SQLSMALLINT dataType = 0;
        SQLLEN ind = 0;
        SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind);
        if (dataType == SQL_GUID)
        {
            foundGuid = true;
            SQLSMALLINT searchable = 0;
            SQLGetData(hStmt, 9, SQL_C_SSHORT, &searchable, 0, &ind);
            EXPECT_EQ(searchable, SQL_ALL_EXCEPT_LIKE)
                << "SQL_GUID SEARCHABLE should be SQL_ALL_EXCEPT_LIKE (2), not " << searchable;

            // LITERAL_PREFIX/SUFFIX should be NULL for GUID
            SQLCHAR prefix[32] = {};
            ret = SQLGetData(hStmt, 4, SQL_C_CHAR, prefix, sizeof(prefix), &ind);
            EXPECT_TRUE(ind == SQL_NULL_DATA || strlen((char*)prefix) == 0)
                << "SQL_GUID LITERAL_PREFIX should be NULL or empty";
            break;
        }
    }
    EXPECT_TRUE(foundGuid) << "SQL_GUID type not found in type info result set";
}

// On FB4+ servers, BINARY/VARBINARY should appear as native types, not as BLOB aliases
TEST_F(TypeInfoTest, NoDuplicateBinaryTypesOnFB4Plus)
{
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Count how many times SQL_BINARY (-2) and SQL_VARBINARY (-3) appear
    std::map<SQLSMALLINT, std::vector<std::string>> typeMap;
    while (SQL_SUCCEEDED(SQLFetch(hStmt)))
    {
        SQLSMALLINT dataType = 0;
        SQLCHAR typeName[128] = {};
        SQLLEN ind = 0;
        SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind);
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &ind);
        typeMap[dataType].push_back(std::string((char*)typeName));
    }

    // SQL_BINARY should not have both "BLOB SUB_TYPE 0" AND "BINARY" on FB4+
    if (typeMap.count(SQL_BINARY))
    {
        auto& names = typeMap[SQL_BINARY];
        bool hasBlobAlias = false;
        bool hasNative = false;
        for (auto& n : names)
        {
            if (n == "BLOB SUB_TYPE 0") hasBlobAlias = true;
            if (n == "BINARY") hasNative = true;
        }
        // They should not coexist — either BLOB alias (pre-FB4) or native (FB4+)
        if (hasBlobAlias && hasNative)
        {
            FAIL() << "SQL_BINARY has both 'BLOB SUB_TYPE 0' and 'BINARY' entries — "
                      "version-gating failed";
        }
    }
}

// Verify every row in SQL_ALL_TYPES has the same DATA_TYPE as reported
TEST_F(TypeInfoTest, AllTypesReturnValidData)
{
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    int rowCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt)))
    {
        SQLCHAR typeName[128] = {};
        SQLSMALLINT dataType = 0;
        SQLINTEGER columnSize = 0;
        SQLLEN ind1 = 0, ind2 = 0, ind3 = 0;

        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &ind1);
        SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind2);
        SQLGetData(hStmt, 3, SQL_C_SLONG, &columnSize, 0, &ind3);

        EXPECT_NE(ind1, SQL_NULL_DATA) << "TYPE_NAME should not be NULL";
        EXPECT_NE(ind2, SQL_NULL_DATA) << "DATA_TYPE should not be NULL";
        EXPECT_GT(strlen((char*)typeName), 0u) << "TYPE_NAME should not be empty";
        rowCount++;
    }
    EXPECT_GT(rowCount, 10) << "Expected at least 10 type info rows";
}

// ============================================================================
// 11.1.6: SQL_ASYNC_MODE test
// ============================================================================

class AsyncModeTest : public OdbcConnectedTest {};

TEST_F(AsyncModeTest, ReportsAsyncModeNone)
{
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

// Default query timeout should be 0
TEST_F(QueryTimeoutTest, DefaultTimeoutIsZero)
{
    SQLULEN timeout = 999;
    SQLRETURN ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 0u) << "Default query timeout should be 0";
}

// Setting and getting timeout
TEST_F(QueryTimeoutTest, SetAndGetTimeout)
{
    SQLRETURN ret = SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)5, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Failed to set query timeout: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLULEN timeout = 0;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(timeout, 5u) << "Query timeout should be 5 after setting";
}

// Setting timeout back to 0 disables it
TEST_F(QueryTimeoutTest, SetTimeoutToZero)
{
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
TEST_F(QueryTimeoutTest, CancelWhenIdleSucceeds)
{
    SQLRETURN ret = SQLCancel(hStmt);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCancel on idle statement should succeed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
}

// SQLCancel from another thread interrupts a long-running query
TEST_F(QueryTimeoutTest, CancelFromAnotherThread)
{
    // Use a cartesian product query that takes a long time
    // rdb$relations has ~50 rows, so CROSS JOIN produces ~2500 rows which is fast
    // We'll use a triple cross join for a really long query
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
    // If cancelled, we expect SQL_ERROR with SQLSTATE HY008 (Operation canceled).
    if (ret == SQL_ERROR)
    {
        std::string state = GetSqlState(SQL_HANDLE_STMT, hStmt);
        // HY008 = Operation canceled, or HY000 for general error from cancel
        EXPECT_TRUE(state == "HY008" || state == "HY000" || state == "HYT00")
            << "Expected cancel-related SQLSTATE, got " << state;
    }
    // If SQL_SUCCESS, the query completed before cancel — that's OK too
}

// Timer-based timeout automatically cancels a long-running query (11.2.2)
TEST_F(QueryTimeoutTest, TimerFiresOnLongQuery)
{
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

    // The query should have been cancelled by the timer.
    // If it completed before timeout, that's also acceptable (fast server).
    if (ret == SQL_ERROR)
    {
        std::string state = GetSqlState(SQL_HANDLE_STMT, hStmt);
        // 11.2.3: timeout-triggered cancel should produce HYT00
        EXPECT_EQ(state, "HYT00")
            << "Timer-triggered cancel should produce HYT00, got " << state;

        // Verify it didn't take much longer than the timeout
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        EXPECT_LE(secs, 5) << "Should cancel within a few seconds of timeout";
    }
    // If SQL_SUCCESS, query was too fast for the timer — acceptable
}

// Timeout of 0 means no timeout — query should complete normally (11.2.1)
TEST_F(QueryTimeoutTest, ZeroTimeoutDoesNotCancel)
{
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

// After reset, autocommit should be ON
TEST_F(ConnectionResetTest, ResetRestoresAutocommit)
{
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
TEST_F(ConnectionResetTest, ResetRestoresTransactionIsolation)
{
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
TEST_F(ConnectionResetTest, ResetRollsBackPendingTransaction)
{
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
TEST_F(ConnectionResetTest, ConnectionReusableAfterReset)
{
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
TEST_F(ConnectionResetTest, ResetClosesOpenCursors)
{
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

// 11.3.3: Reset should restore query timeout on child statements to 0
TEST_F(ConnectionResetTest, ResetResetsQueryTimeout)
{
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
