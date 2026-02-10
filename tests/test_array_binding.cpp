// test_array_binding.cpp — Tests for ODBC "Array of Parameter Values"
// Ported from psqlodbc arraybinding-test.c and params-batch-exec-test.c
// Covers: SQL_ATTR_PARAMSET_SIZE, SQL_ATTR_PARAM_BIND_TYPE,
//         SQL_ATTR_PARAM_STATUS_PTR, SQL_ATTR_PARAMS_PROCESSED_PTR,
//         SQL_ATTR_PARAM_OPERATION_PTR, column-wise binding, row-wise binding
#include "test_helpers.h"

#include <array>
#include <vector>

// ============================================================================
// ArrayBindingTest: ODBC Array of Parameter Values
// ============================================================================
class ArrayBindingTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (hDbc == SQL_NULL_HDBC) return;

        ExecIgnoreError("DROP TABLE ARRAY_BIND_TEST");
        Commit();
        ReallocStmt();
        ExecDirect("CREATE TABLE ARRAY_BIND_TEST (I INTEGER NOT NULL, T VARCHAR(100))");
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        if (hDbc != SQL_NULL_HDBC) {
            ExecIgnoreError("DROP TABLE ARRAY_BIND_TEST");
            SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
        }
        OdbcConnectedTest::TearDown();
    }

    // Helper: count rows in the table
    int CountRows() {
        SQLHSTMT hStmt2 = SQL_NULL_HSTMT;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt2);
        SQLExecDirect(hStmt2, (SQLCHAR*)"SELECT COUNT(*) FROM ARRAY_BIND_TEST", SQL_NTS);
        SQLINTEGER count = 0;
        SQLLEN ind = 0;
        SQLBindCol(hStmt2, 1, SQL_C_SLONG, &count, sizeof(count), &ind);
        SQLFetch(hStmt2);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
        return count;
    }

    // Helper: get a value for a specific row
    std::string GetValue(int id) {
        SQLHSTMT hStmt2 = SQL_NULL_HSTMT;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt2);
        SQLCHAR sql[256];
        sprintf((char*)sql, "SELECT T FROM ARRAY_BIND_TEST WHERE I = %d", id);
        SQLExecDirect(hStmt2, sql, SQL_NTS);
        SQLCHAR buf[101] = {};
        SQLLEN ind = 0;
        SQLBindCol(hStmt2, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
        SQLRETURN r = SQLFetch(hStmt2);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
        if (r == SQL_NO_DATA) return "";
        return std::string((char*)buf);
    }
};

// ============================================================================
// 1. Column-wise binding — basic INSERT
//    (ported from psqlodbc arraybinding-test.c test 1)
// ============================================================================
TEST_F(ArrayBindingTest, ColumnWiseInsert) {
    const int ARRAY_SIZE = 100;
    SQLRETURN ret;

    SQLUINTEGER int_array[ARRAY_SIZE];
    SQLCHAR str_array[ARRAY_SIZE][30];
    SQLLEN int_ind_array[ARRAY_SIZE];
    SQLLEN str_ind_array[ARRAY_SIZE];
    SQLUSMALLINT status_array[ARRAY_SIZE];
    SQLULEN nprocessed = 0;

    for (int i = 0; i < ARRAY_SIZE; i++) {
        int_array[i] = i;
        int_ind_array[i] = 0;
        sprintf((char*)str_array[i], "columnwise %d", i);
        str_ind_array[i] = SQL_NTS;
    }

    // Column-wise binding (the default)
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 5, 0,
                           int_array, 0, int_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 29, 0,
                           str_array, 30, str_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Verify all rows were processed
    EXPECT_EQ(nprocessed, (SQLULEN)ARRAY_SIZE);

    // Verify no errors in status array
    int errorCount = 0;
    for (int i = 0; i < (int)nprocessed; i++) {
        if (status_array[i] != SQL_PARAM_SUCCESS && status_array[i] != SQL_PARAM_SUCCESS_WITH_INFO) {
            errorCount++;
        }
    }
    EXPECT_EQ(errorCount, 0);

    Commit();

    // Verify row count
    EXPECT_EQ(CountRows(), ARRAY_SIZE);

    // Verify some specific values
    EXPECT_EQ(GetValue(0), "columnwise 0");
    EXPECT_EQ(GetValue(1), "columnwise 1");
    EXPECT_EQ(GetValue(50), "columnwise 50");
    EXPECT_EQ(GetValue(99), "columnwise 99");
}

// ============================================================================
// 2. Column-wise binding — using SQLPrepare + SQLExecute
// ============================================================================
TEST_F(ArrayBindingTest, ColumnWisePrepareExecute) {
    const int ARRAY_SIZE = 10;
    SQLRETURN ret;

    SQLINTEGER int_array[ARRAY_SIZE];
    SQLCHAR str_array[ARRAY_SIZE][20];
    SQLLEN int_ind_array[ARRAY_SIZE];
    SQLLEN str_ind_array[ARRAY_SIZE];
    SQLUSMALLINT status_array[ARRAY_SIZE];
    SQLULEN nprocessed = 0;

    for (int i = 0; i < ARRAY_SIZE; i++) {
        int_array[i] = (i + 1) * 10;
        int_ind_array[i] = 0;
        sprintf((char*)str_array[i], "prep %d", i);
        str_ind_array[i] = SQL_NTS;
    }

    // Set PARAMSET_SIZE AFTER SQLPrepare to test execute-time routing
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Prepare first — PARAMSET_SIZE is still 1 at this point
    ret = SQLPrepare(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // NOW set PARAMSET_SIZE > 1 AFTER prepare
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           int_array, 0, int_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 19, 0,
                           str_array, 20, str_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_EQ(nprocessed, (SQLULEN)ARRAY_SIZE);

    for (int i = 0; i < (int)nprocessed; i++) {
        EXPECT_TRUE(status_array[i] == SQL_PARAM_SUCCESS ||
                    status_array[i] == SQL_PARAM_SUCCESS_WITH_INFO)
            << "Row " << i << " status: " << status_array[i];
    }

    Commit();
    EXPECT_EQ(CountRows(), ARRAY_SIZE);
    EXPECT_EQ(GetValue(10), "prep 0");
    EXPECT_EQ(GetValue(100), "prep 9");
}

// ============================================================================
// 3. Row-wise binding — basic INSERT
// ============================================================================
TEST_F(ArrayBindingTest, RowWiseInsert) {
    const int ARRAY_SIZE = 5;
    SQLRETURN ret;

    struct ParamRow {
        SQLINTEGER i;
        SQLLEN     iInd;
        SQLCHAR    t[51];
        SQLLEN     tInd;
    };

    ParamRow rows[ARRAY_SIZE] = {};
    rows[0] = {1, 0, "Alpha",   SQL_NTS};
    rows[1] = {2, 0, "Bravo",   SQL_NTS};
    rows[2] = {3, 0, "Charlie", SQL_NTS};
    rows[3] = {4, 0, "Delta",   SQL_NTS};
    rows[4] = {5, 0, "Echo",    SQL_NTS};

    SQLUSMALLINT status_array[ARRAY_SIZE] = {};
    SQLULEN nprocessed = 0;

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)(intptr_t)sizeof(ParamRow), 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLPrepare(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &rows[0].i, sizeof(rows[0].i), &rows[0].iInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
                           50, 0, rows[0].t, 51, &rows[0].tInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_EQ(nprocessed, (SQLULEN)ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        EXPECT_TRUE(status_array[i] == SQL_PARAM_SUCCESS ||
                    status_array[i] == SQL_PARAM_SUCCESS_WITH_INFO)
            << "Row " << i << " status: " << status_array[i];
    }

    Commit();
    EXPECT_EQ(CountRows(), ARRAY_SIZE);
    EXPECT_EQ(GetValue(1), "Alpha");
    EXPECT_EQ(GetValue(3), "Charlie");
    EXPECT_EQ(GetValue(5), "Echo");
}

// ============================================================================
// 4. Column-wise binding with NULL values
// ============================================================================
TEST_F(ArrayBindingTest, ColumnWiseWithNulls) {
    const int ARRAY_SIZE = 5;
    SQLRETURN ret;

    SQLINTEGER int_array[ARRAY_SIZE] = {1, 2, 3, 4, 5};
    SQLCHAR str_array[ARRAY_SIZE][20] = {"one", "", "three", "", "five"};
    SQLLEN int_ind_array[ARRAY_SIZE] = {0, 0, 0, 0, 0};
    SQLLEN str_ind_array[ARRAY_SIZE] = {SQL_NTS, SQL_NULL_DATA, SQL_NTS, SQL_NULL_DATA, SQL_NTS};
    SQLUSMALLINT status_array[ARRAY_SIZE] = {};
    SQLULEN nprocessed = 0;

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           int_array, 0, int_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 19, 0,
                           str_array, 20, str_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_EQ(nprocessed, (SQLULEN)ARRAY_SIZE);
    Commit();
    EXPECT_EQ(CountRows(), ARRAY_SIZE);

    // Non-null rows
    EXPECT_EQ(GetValue(1), "one");
    EXPECT_EQ(GetValue(3), "three");
    EXPECT_EQ(GetValue(5), "five");

    // Null rows — GetValue returns "" for both NULL and not-found
    // Verify with explicit NULL check
    {
        SQLHSTMT hStmt2 = SQL_NULL_HSTMT;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt2);
        SQLExecDirect(hStmt2, (SQLCHAR*)"SELECT T FROM ARRAY_BIND_TEST WHERE I = 2", SQL_NTS);
        SQLCHAR buf[20] = {};
        SQLLEN ind = 0;
        SQLBindCol(hStmt2, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
        SQLRETURN r = SQLFetch(hStmt2);
        ASSERT_TRUE(SQL_SUCCEEDED(r));
        EXPECT_EQ(ind, SQL_NULL_DATA);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
    }
}

// ============================================================================
// 5. SQL_ATTR_PARAM_OPERATION_PTR — skip individual rows
// ============================================================================
TEST_F(ArrayBindingTest, ParamOperationPtrSkipRows) {
    const int ARRAY_SIZE = 5;
    SQLRETURN ret;

    SQLINTEGER int_array[ARRAY_SIZE] = {10, 20, 30, 40, 50};
    SQLCHAR str_array[ARRAY_SIZE][20] = {"A", "B", "C", "D", "E"};
    SQLLEN int_ind_array[ARRAY_SIZE] = {0, 0, 0, 0, 0};
    SQLLEN str_ind_array[ARRAY_SIZE] = {SQL_NTS, SQL_NTS, SQL_NTS, SQL_NTS, SQL_NTS};
    SQLUSMALLINT status_array[ARRAY_SIZE] = {};
    SQLULEN nprocessed = 0;

    // Skip rows 1 (i=20) and 3 (i=40)
    SQLUSMALLINT operation_array[ARRAY_SIZE] = {
        SQL_PARAM_PROCEED,  // row 0: i=10 — process
        SQL_PARAM_IGNORE,   // row 1: i=20 — skip
        SQL_PARAM_PROCEED,  // row 2: i=30 — process
        SQL_PARAM_IGNORE,   // row 3: i=40 — skip
        SQL_PARAM_PROCEED   // row 4: i=50 — process
    };

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_OPERATION_PTR, operation_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           int_array, 0, int_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 19, 0,
                           str_array, 20, str_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Only 3 rows should have been processed
    EXPECT_EQ(nprocessed, (SQLULEN)3);

    // Skipped rows should remain SQL_PARAM_UNUSED
    EXPECT_TRUE(status_array[0] == SQL_PARAM_SUCCESS || status_array[0] == SQL_PARAM_SUCCESS_WITH_INFO);
    EXPECT_EQ(status_array[1], SQL_PARAM_UNUSED);
    EXPECT_TRUE(status_array[2] == SQL_PARAM_SUCCESS || status_array[2] == SQL_PARAM_SUCCESS_WITH_INFO);
    EXPECT_EQ(status_array[3], SQL_PARAM_UNUSED);
    EXPECT_TRUE(status_array[4] == SQL_PARAM_SUCCESS || status_array[4] == SQL_PARAM_SUCCESS_WITH_INFO);

    Commit();

    // Only 3 rows inserted (10, 30, 50)
    EXPECT_EQ(CountRows(), 3);
    EXPECT_EQ(GetValue(10), "A");
    EXPECT_EQ(GetValue(30), "C");
    EXPECT_EQ(GetValue(50), "E");

    // Skipped rows should not exist
    EXPECT_EQ(GetValue(20), "");
    EXPECT_EQ(GetValue(40), "");
}

// ============================================================================
// 6. Large array — column-wise (like psqlodbc's 10000-row test)
// ============================================================================
TEST_F(ArrayBindingTest, LargeColumnWiseArray) {
    const int ARRAY_SIZE = 1000;
    SQLRETURN ret;

    std::vector<SQLUINTEGER> int_array(ARRAY_SIZE);
    std::vector<std::array<SQLCHAR, 40>> str_array(ARRAY_SIZE);
    std::vector<SQLLEN> int_ind_array(ARRAY_SIZE, 0);
    std::vector<SQLLEN> str_ind_array(ARRAY_SIZE, SQL_NTS);
    std::vector<SQLUSMALLINT> status_array(ARRAY_SIZE, 0);
    SQLULEN nprocessed = 0;

    for (int i = 0; i < ARRAY_SIZE; i++) {
        int_array[i] = i;
        sprintf((char*)str_array[i].data(), "row %d", i);
    }

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array.data(), 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 5, 0,
                           int_array.data(), 0, int_ind_array.data());
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 39, 0,
                           str_array.data(), 40, str_ind_array.data());
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_EQ(nprocessed, (SQLULEN)ARRAY_SIZE);

    int errorCount = 0;
    for (int i = 0; i < (int)nprocessed; i++) {
        if (status_array[i] != SQL_PARAM_SUCCESS && status_array[i] != SQL_PARAM_SUCCESS_WITH_INFO)
            errorCount++;
    }
    EXPECT_EQ(errorCount, 0);

    Commit();
    EXPECT_EQ(CountRows(), ARRAY_SIZE);

    // Spot-check values
    EXPECT_EQ(GetValue(0), "row 0");
    EXPECT_EQ(GetValue(500), "row 500");
    EXPECT_EQ(GetValue(999), "row 999");
}

// ============================================================================
// 7. Re-execute array batch with different data
//    (from psqlodbc params-batch-exec-test.c)
// ============================================================================
TEST_F(ArrayBindingTest, ReExecuteWithDifferentData) {
    const int BATCH_SIZE = 5;
    SQLRETURN ret;

    SQLINTEGER int_array[BATCH_SIZE] = {1, 2, 3, 4, 5};
    SQLCHAR str_array[BATCH_SIZE][20] = {"A1", "B1", "C1", "D1", "E1"};
    SQLLEN int_ind_array[BATCH_SIZE] = {0, 0, 0, 0, 0};
    SQLLEN str_ind_array[BATCH_SIZE] = {SQL_NTS, SQL_NTS, SQL_NTS, SQL_NTS, SQL_NTS};
    SQLUSMALLINT status_array[BATCH_SIZE] = {};
    SQLULEN nprocessed = 0;

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)BATCH_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           int_array, 0, int_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 19, 0,
                           str_array, 20, str_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // First execution
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(nprocessed, (SQLULEN)BATCH_SIZE);

    Commit();
    EXPECT_EQ(CountRows(), BATCH_SIZE);

    // Change data and re-execute
    for (int i = 0; i < BATCH_SIZE; i++) {
        int_array[i] = (i + 1) * 100;
        sprintf((char*)str_array[i], "re-exec %d", i);
    }

    ReallocStmt();
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)BATCH_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           int_array, 0, int_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 19, 0,
                           str_array, 20, str_ind_array);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(nprocessed, (SQLULEN)BATCH_SIZE);

    Commit();
    EXPECT_EQ(CountRows(), 2 * BATCH_SIZE);
    EXPECT_EQ(GetValue(100), "re-exec 0");
    EXPECT_EQ(GetValue(500), "re-exec 4");
}

// ============================================================================
// 8. New handle required after array execution for non-array queries
//    (from psqlodbc arraybinding-test.c: "parameters set with
//     SQLSetStmtAttr survive SQLFreeStmt")
// ============================================================================
TEST_F(ArrayBindingTest, NewHandleAfterArrayExec) {
    const int ARRAY_SIZE = 3;
    SQLRETURN ret;

    SQLINTEGER int_array[ARRAY_SIZE] = {1, 2, 3};
    SQLCHAR str_array[ARRAY_SIZE][10] = {"a", "b", "c"};
    SQLLEN int_ind[ARRAY_SIZE] = {0, 0, 0};
    SQLLEN str_ind[ARRAY_SIZE] = {SQL_NTS, SQL_NTS, SQL_NTS};

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           int_array, 0, int_ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 9, 0,
                           str_array, 10, str_ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    Commit();

    // Free and allocate a new handle for non-array query
    // (psqlodbc: "parameters set with SQLSetStmtAttr survive SQLFreeStmt")
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Non-array SELECT
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT COUNT(*) FROM ARRAY_BIND_TEST", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER count = 0;
    SQLLEN countInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, sizeof(count), &countInd);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, ARRAY_SIZE);
}

// ============================================================================
// 9. Row-wise binding with multiple data types
// ============================================================================
TEST_F(ArrayBindingTest, RowWiseMultipleTypes) {
    // Drop and recreate with additional columns
    ExecIgnoreError("DROP TABLE ARRAY_BIND_TEST");
    Commit();
    ReallocStmt();
    ExecDirect("CREATE TABLE ARRAY_BIND_TEST (I INTEGER NOT NULL, F DOUBLE PRECISION, T VARCHAR(50))");
    Commit();
    ReallocStmt();

    const int ARRAY_SIZE = 3;
    SQLRETURN ret;

    struct ParamRow {
        SQLINTEGER i;
        SQLLEN     iInd;
        SQLDOUBLE  f;
        SQLLEN     fInd;
        SQLCHAR    t[51];
        SQLLEN     tInd;
    };

    ParamRow rows[ARRAY_SIZE] = {};
    rows[0] = {1, 0, 3.14,  0, "pi",    SQL_NTS};
    rows[1] = {2, 0, 2.718, 0, "euler", SQL_NTS};
    rows[2] = {3, 0, 1.414, 0, "sqrt2", SQL_NTS};

    SQLUSMALLINT status_array[ARRAY_SIZE] = {};
    SQLULEN nprocessed = 0;

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)(intptr_t)sizeof(ParamRow), 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLPrepare(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, F, T) VALUES (?, ?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &rows[0].i, sizeof(rows[0].i), &rows[0].iInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,
                           15, 0, &rows[0].f, sizeof(rows[0].f), &rows[0].fInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
                           50, 0, rows[0].t, 51, &rows[0].tInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_EQ(nprocessed, (SQLULEN)ARRAY_SIZE);
    Commit();

    // Verify
    SQLHSTMT hStmt2 = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt2);
    SQLExecDirect(hStmt2, (SQLCHAR*)"SELECT I, F, T FROM ARRAY_BIND_TEST ORDER BY I", SQL_NTS);
    SQLINTEGER ival;
    SQLDOUBLE fval;
    SQLCHAR tval[51];
    SQLLEN iInd, fInd, tInd;
    SQLBindCol(hStmt2, 1, SQL_C_SLONG, &ival, sizeof(ival), &iInd);
    SQLBindCol(hStmt2, 2, SQL_C_DOUBLE, &fval, sizeof(fval), &fInd);
    SQLBindCol(hStmt2, 3, SQL_C_CHAR, tval, sizeof(tval), &tInd);

    ret = SQLFetch(hStmt2);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ival, 1);
    EXPECT_NEAR(fval, 3.14, 0.001);
    EXPECT_STREQ((char*)tval, "pi");

    ret = SQLFetch(hStmt2);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ival, 2);
    EXPECT_NEAR(fval, 2.718, 0.001);
    EXPECT_STREQ((char*)tval, "euler");

    ret = SQLFetch(hStmt2);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ival, 3);
    EXPECT_NEAR(fval, 1.414, 0.001);
    EXPECT_STREQ((char*)tval, "sqrt2");

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
}

// ============================================================================
// 10. Column-wise binding with UPDATE statement
// ============================================================================
TEST_F(ArrayBindingTest, ColumnWiseUpdate) {
    // Insert some initial data
    {
        SQLHSTMT hStmt2 = SQL_NULL_HSTMT;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt2);
        SQLExecDirect(hStmt2, (SQLCHAR*)
            "INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (1, 'old1')", SQL_NTS);
        SQLExecDirect(hStmt2, (SQLCHAR*)
            "INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (2, 'old2')", SQL_NTS);
        SQLExecDirect(hStmt2, (SQLCHAR*)
            "INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (3, 'old3')", SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
    }
    Commit();
    ReallocStmt();

    const int ARRAY_SIZE = 3;
    SQLRETURN ret;

    // UPDATE: SET T = ? WHERE I = ?
    SQLCHAR new_vals[ARRAY_SIZE][20] = {"new1", "new2", "new3"};
    SQLINTEGER ids[ARRAY_SIZE] = {1, 2, 3};
    SQLLEN val_ind[ARRAY_SIZE] = {SQL_NTS, SQL_NTS, SQL_NTS};
    SQLLEN id_ind[ARRAY_SIZE] = {0, 0, 0};
    SQLUSMALLINT status_array[ARRAY_SIZE] = {};
    SQLULEN nprocessed = 0;

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 19, 0,
                           new_vals, 20, val_ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           ids, 0, id_ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)
        "UPDATE ARRAY_BIND_TEST SET T = ? WHERE I = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_EQ(nprocessed, (SQLULEN)ARRAY_SIZE);
    Commit();

    EXPECT_EQ(GetValue(1), "new1");
    EXPECT_EQ(GetValue(2), "new2");
    EXPECT_EQ(GetValue(3), "new3");
}

// ============================================================================
// 11. Column-wise binding with DELETE statement
// ============================================================================
TEST_F(ArrayBindingTest, ColumnWiseDelete) {
    // Insert initial data
    {
        SQLHSTMT hStmt2 = SQL_NULL_HSTMT;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt2);
        for (int i = 1; i <= 5; i++) {
            SQLCHAR sql[128];
            sprintf((char*)sql, "INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (%d, 'val%d')", i, i);
            SQLExecDirect(hStmt2, sql, SQL_NTS);
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
    }
    Commit();
    ReallocStmt();

    // Delete rows 2 and 4 using array binding
    const int ARRAY_SIZE = 2;
    SQLINTEGER ids[ARRAY_SIZE] = {2, 4};
    SQLLEN id_ind[ARRAY_SIZE] = {0, 0};
    SQLUSMALLINT status_array[ARRAY_SIZE] = {};
    SQLULEN nprocessed = 0;
    SQLRETURN ret;

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status_array, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           ids, 0, id_ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"DELETE FROM ARRAY_BIND_TEST WHERE I = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_EQ(nprocessed, (SQLULEN)ARRAY_SIZE);
    Commit();

    // Only rows 1, 3, 5 should remain
    EXPECT_EQ(CountRows(), 3);
    EXPECT_EQ(GetValue(1), "val1");
    EXPECT_EQ(GetValue(2), "");  // deleted
    EXPECT_EQ(GetValue(3), "val3");
    EXPECT_EQ(GetValue(4), "");  // deleted
    EXPECT_EQ(GetValue(5), "val5");
}

// ============================================================================
// 12. PARAMSET_SIZE = 1 should behave like normal single execution
// ============================================================================
TEST_F(ArrayBindingTest, ParamsetSizeOneIsNormal) {
    SQLRETURN ret;

    // Use column-wise with size=1 — should still work
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)1, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER id = 42;
    SQLLEN idInd = 0;
    SQLCHAR val[] = "single-row";
    SQLLEN valInd = SQL_NTS;

    ret = SQLPrepare(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           &id, 0, &idInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0,
                           val, sizeof(val), &valInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    Commit();
    EXPECT_EQ(CountRows(), 1);
    EXPECT_EQ(GetValue(42), "single-row");
}

// ============================================================================
// 13. SQLGetInfo reports SQL_PARC_BATCH for SQL_PARAM_ARRAY_ROW_COUNTS
// ============================================================================
TEST_F(ArrayBindingTest, GetInfoParamArrayRowCounts) {
    SQLUINTEGER value = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_PARAM_ARRAY_ROW_COUNTS, &value, sizeof(value), NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(value, (SQLUINTEGER)SQL_PARC_BATCH);
}

// ============================================================================
// 14. SQLGetInfo reports SQL_PAS_BATCH for SQL_PARAM_ARRAY_SELECTS
// ============================================================================
TEST_F(ArrayBindingTest, GetInfoParamArraySelects) {
    SQLUINTEGER value = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_PARAM_ARRAY_SELECTS, &value, sizeof(value), NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(value, (SQLUINTEGER)SQL_PAS_BATCH);
}

// ============================================================================
// 15. Column-wise binding with integer-only (no strings)
// ============================================================================
TEST_F(ArrayBindingTest, ColumnWiseIntegerOnly) {
    // Recreate table with integer-only columns
    ExecIgnoreError("DROP TABLE ARRAY_BIND_TEST");
    Commit();
    ReallocStmt();
    ExecDirect("CREATE TABLE ARRAY_BIND_TEST (I INTEGER NOT NULL, T INTEGER)");
    Commit();
    ReallocStmt();

    const int ARRAY_SIZE = 10;
    SQLINTEGER i_array[ARRAY_SIZE];
    SQLINTEGER t_array[ARRAY_SIZE];
    SQLLEN i_ind[ARRAY_SIZE];
    SQLLEN t_ind[ARRAY_SIZE];
    SQLULEN nprocessed = 0;
    SQLRETURN ret;

    for (int i = 0; i < ARRAY_SIZE; i++) {
        i_array[i] = i + 1;
        t_array[i] = (i + 1) * 100;
        i_ind[i] = 0;
        t_ind[i] = 0;
    }

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           i_array, 0, i_ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           t_array, 0, t_ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_EQ(nprocessed, (SQLULEN)ARRAY_SIZE);
    Commit();

    // Verify
    SQLHSTMT hStmt2 = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt2);
    SQLExecDirect(hStmt2, (SQLCHAR*)"SELECT I, T FROM ARRAY_BIND_TEST ORDER BY I", SQL_NTS);
    SQLINTEGER ival, tval;
    SQLLEN iInd2, tInd2;
    SQLBindCol(hStmt2, 1, SQL_C_SLONG, &ival, sizeof(ival), &iInd2);
    SQLBindCol(hStmt2, 2, SQL_C_SLONG, &tval, sizeof(tval), &tInd2);

    for (int i = 0; i < ARRAY_SIZE; i++) {
        ret = SQLFetch(hStmt2);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Row " << i;
        EXPECT_EQ(ival, i + 1);
        EXPECT_EQ(tval, (i + 1) * 100);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt2);
}

// ============================================================================
// 16. Row-wise with SQL_ATTR_PARAM_OPERATION_PTR
// ============================================================================
TEST_F(ArrayBindingTest, RowWiseWithOperationPtr) {
    const int ARRAY_SIZE = 4;
    SQLRETURN ret;

    struct ParamRow {
        SQLINTEGER i;
        SQLLEN     iInd;
        SQLCHAR    t[21];
        SQLLEN     tInd;
    };

    ParamRow rows[ARRAY_SIZE] = {};
    rows[0] = {10, 0, "row10", SQL_NTS};
    rows[1] = {20, 0, "row20", SQL_NTS};
    rows[2] = {30, 0, "row30", SQL_NTS};
    rows[3] = {40, 0, "row40", SQL_NTS};

    SQLUSMALLINT operation[ARRAY_SIZE] = {
        SQL_PARAM_PROCEED,  // process
        SQL_PARAM_IGNORE,   // skip
        SQL_PARAM_PROCEED,  // process
        SQL_PARAM_PROCEED   // process
    };
    SQLUSMALLINT status[ARRAY_SIZE] = {};
    SQLULEN nprocessed = 0;

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)(intptr_t)sizeof(ParamRow), 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, status, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &nprocessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_OPERATION_PTR, operation, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLPrepare(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &rows[0].i, sizeof(rows[0].i), &rows[0].iInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
                           20, 0, rows[0].t, 21, &rows[0].tInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_EQ(nprocessed, (SQLULEN)3);  // 3 processed, 1 skipped
    EXPECT_EQ(status[1], SQL_PARAM_UNUSED);  // skipped

    Commit();
    EXPECT_EQ(CountRows(), 3);
    EXPECT_EQ(GetValue(10), "row10");
    EXPECT_EQ(GetValue(20), "");  // was skipped
    EXPECT_EQ(GetValue(30), "row30");
    EXPECT_EQ(GetValue(40), "row40");
}

// ============================================================================
// 17. Without status/processed pointers (optional per spec)
// ============================================================================
TEST_F(ArrayBindingTest, WithoutStatusPointers) {
    const int ARRAY_SIZE = 3;
    SQLRETURN ret;

    SQLINTEGER int_array[ARRAY_SIZE] = {100, 200, 300};
    SQLCHAR str_array[ARRAY_SIZE][20] = {"x1", "x2", "x3"};
    SQLLEN int_ind[ARRAY_SIZE] = {0, 0, 0};
    SQLLEN str_ind[ARRAY_SIZE] = {SQL_NTS, SQL_NTS, SQL_NTS};

    // Set PARAMSET_SIZE but NOT status/processed ptrs
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)ARRAY_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           int_array, 0, int_ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 19, 0,
                           str_array, 20, str_ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ARRAY_BIND_TEST (I, T) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    Commit();
    EXPECT_EQ(CountRows(), ARRAY_SIZE);
}
