// tests/test_data_at_execution.cpp — SQL_DATA_AT_EXEC / SQLPutData tests
// (Phase 6, ported from psqlodbc dataatexecution-test)
//
// Tests the data-at-execution mechanism for sending parameter data at
// execution time via SQLParamData / SQLPutData.

#include "test_helpers.h"
#include <cstring>
#include <vector>

class DataAtExecutionTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_DAE",
            "ID INTEGER NOT NULL PRIMARY KEY, "
            "VAL_TEXT VARCHAR(200), "
            "VAL_BLOB BLOB SUB_TYPE TEXT");

        // Insert reference data
        ExecDirect("INSERT INTO ODBC_TEST_DAE VALUES (1, 'alpha', 'blob-alpha')");
        ExecDirect("INSERT INTO ODBC_TEST_DAE VALUES (2, 'beta', 'blob-beta')");
        ExecDirect("INSERT INTO ODBC_TEST_DAE VALUES (3, 'gamma', 'blob-gamma')");
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

// ===== Basic data-at-execution with VARCHAR =====

TEST_F(DataAtExecutionTest, SingleVarcharParam) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_DAE WHERE VAL_TEXT = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Bind with SQL_DATA_AT_EXEC
    SQLLEN cbParam = SQL_DATA_AT_EXEC;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT,
        SQL_C_CHAR, SQL_VARCHAR, 200, 0,
        (SQLPOINTER)1,  // parameter identifier
        0, &cbParam);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Execute — should return SQL_NEED_DATA
    ret = SQLExecute(hStmt);
    EXPECT_EQ(ret, SQL_NEED_DATA);

    // Provide the data
    SQLPOINTER paramId = nullptr;
    ret = SQLParamData(hStmt, &paramId);
    EXPECT_EQ(ret, SQL_NEED_DATA);

    // Send the actual value
    ret = SQLPutData(hStmt, (SQLPOINTER)"beta", 4);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLPutData failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Call SQLParamData again to complete
    ret = SQLParamData(hStmt, &paramId);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Final SQLParamData failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Fetch the result
    SQLINTEGER id = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(id, 2);
}

// ===== Multiple data-at-execution parameters =====

TEST_F(DataAtExecutionTest, TwoVarcharParams) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_DAE WHERE VAL_TEXT = ? OR VAL_TEXT = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLLEN cbParam1 = SQL_DATA_AT_EXEC;
    SQLLEN cbParam2 = SQL_DATA_AT_EXEC;

    // Bind param 1 with token (void*)1
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT,
        SQL_C_CHAR, SQL_VARCHAR, 200, 0,
        (SQLPOINTER)1, 0, &cbParam1);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Bind param 2 with token (void*)2
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT,
        SQL_C_CHAR, SQL_VARCHAR, 200, 0,
        (SQLPOINTER)2, 0, &cbParam2);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Execute — should return SQL_NEED_DATA
    ret = SQLExecute(hStmt);
    EXPECT_EQ(ret, SQL_NEED_DATA);

    // Provide data for each parameter
    SQLPOINTER paramId = nullptr;
    int paramsProvided = 0;
    while ((ret = SQLParamData(hStmt, &paramId)) == SQL_NEED_DATA) {
        if (paramId == (SQLPOINTER)1) {
            ret = SQLPutData(hStmt, (SQLPOINTER)"alpha", 5);
        } else if (paramId == (SQLPOINTER)2) {
            ret = SQLPutData(hStmt, (SQLPOINTER)"gamma", 5);
        } else {
            FAIL() << "Unexpected parameter ID: " << (intptr_t)paramId;
        }
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "SQLPutData failed";
        paramsProvided++;
    }
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Final SQLParamData failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(paramsProvided, 2);

    // Fetch results — should get 2 rows (id=1 and id=3)
    SQLINTEGER id = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &ind);

    std::vector<int> ids;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        ids.push_back(id);
    }
    EXPECT_EQ(ids.size(), 2u);
    // Order may vary, but should have 1 and 3
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), 1) != ids.end());
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), 3) != ids.end());
}

// ===== Data-at-execution for INSERT =====

TEST_F(DataAtExecutionTest, InsertWithDAE) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_DAE (ID, VAL_TEXT) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER id = 100;
    SQLLEN idInd = sizeof(id);
    SQLLEN textInd = SQL_DATA_AT_EXEC;

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
        0, 0, &id, sizeof(id), &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        200, 0, (SQLPOINTER)2, 0, &textInd);

    ret = SQLExecute(hStmt);
    EXPECT_EQ(ret, SQL_NEED_DATA);

    SQLPOINTER paramId = nullptr;
    ret = SQLParamData(hStmt, &paramId);
    EXPECT_EQ(ret, SQL_NEED_DATA);

    ret = SQLPutData(hStmt, (SQLPOINTER)"inserted-via-dae", 16);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLParamData(hStmt, &paramId);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Final SQLParamData failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    Commit();

    // Verify the insert
    ReallocStmt();
    ExecDirect("SELECT VAL_TEXT FROM ODBC_TEST_DAE WHERE ID = 100");

    SQLCHAR buf[64] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)buf, "inserted-via-dae");
}

// ===== SQLPutData in multiple chunks =====

TEST_F(DataAtExecutionTest, PutDataMultipleChunks) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_DAE (ID, VAL_TEXT) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER id = 200;
    SQLLEN idInd = sizeof(id);
    SQLLEN textInd = SQL_DATA_AT_EXEC;

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
        0, 0, &id, sizeof(id), &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
        200, 0, (SQLPOINTER)2, 0, &textInd);

    ret = SQLExecute(hStmt);
    EXPECT_EQ(ret, SQL_NEED_DATA);

    SQLPOINTER paramId = nullptr;
    ret = SQLParamData(hStmt, &paramId);
    EXPECT_EQ(ret, SQL_NEED_DATA);

    // Send data in multiple chunks
    ret = SQLPutData(hStmt, (SQLPOINTER)"chunk1-", 7);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLPutData(hStmt, (SQLPOINTER)"chunk2-", 7);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLPutData(hStmt, (SQLPOINTER)"chunk3", 6);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLParamData(hStmt, &paramId);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Final SQLParamData failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    Commit();

    // Verify
    ReallocStmt();
    ExecDirect("SELECT VAL_TEXT FROM ODBC_TEST_DAE WHERE ID = 200");

    SQLCHAR buf[64] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)buf, "chunk1-chunk2-chunk3");
}

// ===== Cancel data-at-execution =====

TEST_F(DataAtExecutionTest, CancelDuringDAE) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_DAE WHERE VAL_TEXT = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLLEN cbParam = SQL_DATA_AT_EXEC;
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT,
        SQL_C_CHAR, SQL_VARCHAR, 200, 0,
        (SQLPOINTER)1, 0, &cbParam);

    ret = SQLExecute(hStmt);
    EXPECT_EQ(ret, SQL_NEED_DATA);

    // Cancel instead of providing data
    ret = SQLCancel(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCancel failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Statement should be reusable after cancel
    ReallocStmt();
    ExecDirect("SELECT 1 FROM RDB$DATABASE");
    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 1);
}

// ===== Data-at-execution with BLOB =====

TEST_F(DataAtExecutionTest, BlobDAE) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_DAE (ID, VAL_BLOB) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER id = 300;
    SQLLEN idInd = sizeof(id);
    SQLLEN blobInd = SQL_DATA_AT_EXEC;

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
        0, 0, &id, sizeof(id), &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR,
        1000, 0, (SQLPOINTER)2, 0, &blobInd);

    ret = SQLExecute(hStmt);
    EXPECT_EQ(ret, SQL_NEED_DATA);

    SQLPOINTER paramId = nullptr;
    ret = SQLParamData(hStmt, &paramId);
    EXPECT_EQ(ret, SQL_NEED_DATA);

    // Send blob data in chunks
    std::string blobText = "This is a BLOB text value sent via data-at-execution";
    ret = SQLPutData(hStmt, (SQLPOINTER)blobText.c_str(), (SQLLEN)blobText.size());
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLParamData(hStmt, &paramId);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Final SQLParamData failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    Commit();

    // Verify
    ReallocStmt();
    ExecDirect("SELECT VAL_BLOB FROM ODBC_TEST_DAE WHERE ID = 300");

    SQLCHAR buf[256] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)buf, blobText.c_str());
}
