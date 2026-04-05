// tests/test_blob.cpp — BLOB read/write tests (Phase 3.9)

#include "test_helpers.h"

class BlobTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_BLOB",
            "ID INTEGER NOT NULL PRIMARY KEY, "
            "TEXT_BLOB BLOB SUB_TYPE TEXT, "
            "BIN_BLOB BLOB SUB_TYPE BINARY"
        );
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

TEST_F(BlobTest, SmallTextBlob) {
    ExecDirect("INSERT INTO ODBC_TEST_BLOB (ID, TEXT_BLOB) VALUES (1, 'Hello BLOB World')");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT TEXT_BLOB FROM ODBC_TEST_BLOB WHERE ID = 1");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR val[256] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)val, "Hello BLOB World");
}

TEST_F(BlobTest, LargeTextBlob) {
    // Create a large string (64KB)
    const int SIZE = 64 * 1024;
    std::string largeStr;
    largeStr.reserve(SIZE);
    for (int i = 0; i < SIZE; i++) {
        largeStr += ('A' + (i % 26));
    }

    // Insert using parameter binding
    ReallocStmt();
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_BLOB (ID, TEXT_BLOB) VALUES (2, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLLEN strInd = (SQLLEN)largeStr.size();
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR,
                           largeStr.size(), 0, (SQLPOINTER)largeStr.c_str(), 0, &strInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Large BLOB insert failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    Commit();
    ReallocStmt();

    // Read back via GetData in chunks
    ExecDirect("SELECT TEXT_BLOB FROM ODBC_TEST_BLOB WHERE ID = 2");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    std::string result;
    SQLCHAR buffer[4096];
    SQLLEN ind = 0;

    while (true) {
        ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buffer, sizeof(buffer), &ind);
        if (ret == SQL_NO_DATA)
            break;
        ASSERT_TRUE(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
            << "GetData failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

        if (ind == SQL_NULL_DATA)
            break;

        SQLLEN bytesToAppend;
        if (ret == SQL_SUCCESS_WITH_INFO) {
            // Buffer was filled completely (minus null terminator)
            bytesToAppend = sizeof(buffer) - 1;
        } else {
            bytesToAppend = ind;
        }
        result.append((char*)buffer, bytesToAppend);

        if (ret == SQL_SUCCESS)
            break;
    }

    EXPECT_EQ(result.size(), largeStr.size());
    EXPECT_EQ(result, largeStr);
}

TEST_F(BlobTest, NullBlob) {
    ExecDirect("INSERT INTO ODBC_TEST_BLOB (ID, TEXT_BLOB) VALUES (3, NULL)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT TEXT_BLOB FROM ODBC_TEST_BLOB WHERE ID = 3");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR val[32] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ind, SQL_NULL_DATA);
}

TEST_F(BlobTest, EmptyTextBlob) {
    ExecDirect("INSERT INTO ODBC_TEST_BLOB (ID, TEXT_BLOB) VALUES (4, '')");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT TEXT_BLOB FROM ODBC_TEST_BLOB WHERE ID = 4");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR val[32] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLGetData(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    // Empty BLOB should return 0-length string (not NULL)
    if (ind != SQL_NULL_DATA) {
        EXPECT_EQ(ind, 0) << "Empty BLOB should have indicator 0";
    }
}

TEST_F(BlobTest, ExactBoundaryBlob) {
    // Create a string whose size is exactly kGetDataChunkSize - 1 (4095).
    // This tests the boundary where GetData returns SQL_SUCCESS (not _WITH_INFO)
    // on the first call with a 4096-byte buffer.
    const int SIZE = kGetDataChunkSize - 1;
    std::string boundaryStr;
    boundaryStr.reserve(SIZE);
    for (int i = 0; i < SIZE; i++)
        boundaryStr += ('A' + (i % 26));

    ReallocStmt();
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_BLOB (ID, TEXT_BLOB) VALUES (5, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLLEN strInd = (SQLLEN)boundaryStr.size();
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR,
                           boundaryStr.size(), 0, (SQLPOINTER)boundaryStr.c_str(), 0, &strInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Boundary BLOB insert failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    Commit();
    ReallocStmt();

    ExecDirect("SELECT TEXT_BLOB FROM ODBC_TEST_BLOB WHERE ID = 5");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    SQLCHAR buffer[kGetDataChunkSize];
    SQLLEN ind = 0;
    ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buffer, sizeof(buffer), &ind);
    // With buffer=4096 and data=4095, it should fit in one call
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "GetData failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(std::string((char*)buffer), boundaryStr);
}

TEST_F(BlobTest, BinaryBlobRoundTrip) {
    // Insert binary data with all byte values 0x00..0xFF
    const int SIZE = 256;
    std::vector<unsigned char> binaryData(SIZE);
    for (int i = 0; i < SIZE; i++)
        binaryData[i] = static_cast<unsigned char>(i);

    ReallocStmt();
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_BLOB (ID, BIN_BLOB) VALUES (6, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLLEN binInd = SIZE;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY,
                           SIZE, 0, binaryData.data(), SIZE, &binInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Binary BLOB insert failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    Commit();
    ReallocStmt();

    ExecDirect("SELECT BIN_BLOB FROM ODBC_TEST_BLOB WHERE ID = 6");
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));

    unsigned char readBuf[512] = {};
    SQLLEN ind = 0;
    ret = SQLGetData(hStmt, 1, SQL_C_BINARY, readBuf, sizeof(readBuf), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Binary GetData failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    ASSERT_EQ(ind, SIZE);
    EXPECT_EQ(memcmp(readBuf, binaryData.data(), SIZE), 0)
        << "Binary BLOB data mismatch after round-trip";
}
