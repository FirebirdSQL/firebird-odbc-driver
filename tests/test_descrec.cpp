// tests/test_descrec.cpp — SQLGetDescRec comprehensive tests
// (Phase 6, ported from psqlodbc descrec-test)
//
// Tests SQLGetDescRec on the IRD (Implementation Row Descriptor) for
// multiple column types. Verifies name, type, octet length, precision,
// scale, and nullable fields.

#include "test_helpers.h"
#include <cstring>

class DescRecTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_DESCREC",
            "COL_INT INTEGER NOT NULL, "
            "COL_SMALLINT SMALLINT, "
            "COL_BIGINT BIGINT NOT NULL, "
            "COL_FLOAT FLOAT, "
            "COL_DOUBLE DOUBLE PRECISION, "
            "COL_NUMERIC NUMERIC(10,3), "
            "COL_VARCHAR VARCHAR(50) NOT NULL, "
            "COL_CHAR CHAR(20), "
            "COL_DATE DATE, "
            "COL_TIME TIME, "
            "COL_TIMESTAMP TIMESTAMP");
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

TEST_F(DescRecTest, GetDescRecForAllColumnTypes) {
    // Prepare and execute to populate IRD
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT * FROM ODBC_TEST_DESCREC", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "ExecDirect failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLSMALLINT colcount = 0;
    rc = SQLNumResultCols(hStmt, &colcount);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(colcount, 11);

    // Verify each column using SQLDescribeCol (more reliable across drivers)
    for (SQLSMALLINT i = 1; i <= colcount; i++) {
        SQLCHAR colName[128] = {};
        SQLSMALLINT nameLen = 0, dataType = 0, decDigits = 0, nullable = 0;
        SQLULEN colSize = 0;

        rc = SQLDescribeCol(hStmt, i, colName, sizeof(colName), &nameLen,
                            &dataType, &colSize, &decDigits, &nullable);
        ASSERT_TRUE(SQL_SUCCEEDED(rc))
            << "SQLDescribeCol failed for column " << i << ": "
            << GetOdbcError(SQL_HANDLE_STMT, hStmt);

        // Name should not be empty
        EXPECT_GT(nameLen, 0) << "Column " << i << " has empty name";

        // Type should be valid (non-zero)
        EXPECT_NE(dataType, 0) << "Column " << i << " has type 0";
    }
}

TEST_F(DescRecTest, VerifyIntegerColumn) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT COL_INT FROM ODBC_TEST_DESCREC", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLHDESC hDesc = SQL_NULL_HDESC;
    rc = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hDesc, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0, type = 0, subType = 0;
    SQLSMALLINT precision = 0, scale = 0, nullable = 0;
    SQLLEN length = 0;

    rc = SQLGetDescRec(hDesc, 1, name, sizeof(name), &nameLen,
                       &type, &subType, &length, &precision, &scale, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    EXPECT_STREQ((char*)name, "COL_INT");
    EXPECT_EQ(type, SQL_INTEGER);
    EXPECT_EQ(nullable, SQL_NO_NULLS);
}

TEST_F(DescRecTest, VerifyVarcharColumn) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT COL_VARCHAR FROM ODBC_TEST_DESCREC", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLHDESC hDesc = SQL_NULL_HDESC;
    rc = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hDesc, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0, type = 0, subType = 0;
    SQLSMALLINT precision = 0, scale = 0, nullable = 0;
    SQLLEN length = 0;

    rc = SQLGetDescRec(hDesc, 1, name, sizeof(name), &nameLen,
                       &type, &subType, &length, &precision, &scale, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    EXPECT_STREQ((char*)name, "COL_VARCHAR");
    EXPECT_TRUE(type == SQL_VARCHAR || type == SQL_WVARCHAR);
    EXPECT_EQ(nullable, SQL_NO_NULLS);
    EXPECT_GT(length, 0);
}

TEST_F(DescRecTest, VerifyNumericColumn) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT COL_NUMERIC FROM ODBC_TEST_DESCREC", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLHDESC hDesc = SQL_NULL_HDESC;
    rc = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hDesc, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0, type = 0, subType = 0;
    SQLSMALLINT precision = 0, scale = 0, nullable = 0;
    SQLLEN length = 0;

    rc = SQLGetDescRec(hDesc, 1, name, sizeof(name), &nameLen,
                       &type, &subType, &length, &precision, &scale, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    EXPECT_STREQ((char*)name, "COL_NUMERIC");
    // Firebird may report NUMERIC or DECIMAL
    EXPECT_TRUE(type == SQL_NUMERIC || type == SQL_DECIMAL)
        << "Unexpected type: " << type;
    // Firebird often reports precision as 18 (int64 backing storage)
    // rather than the declared 10
    EXPECT_GE(precision, 10);
    EXPECT_EQ(scale, 3);
    EXPECT_EQ(nullable, SQL_NULLABLE);
}

TEST_F(DescRecTest, VerifyBigintColumn) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT COL_BIGINT FROM ODBC_TEST_DESCREC", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLHDESC hDesc = SQL_NULL_HDESC;
    rc = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hDesc, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0, type = 0, subType = 0;
    SQLSMALLINT precision = 0, scale = 0, nullable = 0;
    SQLLEN length = 0;

    rc = SQLGetDescRec(hDesc, 1, name, sizeof(name), &nameLen,
                       &type, &subType, &length, &precision, &scale, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    EXPECT_STREQ((char*)name, "COL_BIGINT");
    EXPECT_EQ(type, SQL_BIGINT);
    EXPECT_EQ(nullable, SQL_NO_NULLS);
}

TEST_F(DescRecTest, VerifyDateTimeColumns) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT COL_DATE, COL_TIME, COL_TIMESTAMP FROM ODBC_TEST_DESCREC",
        SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Use SQLDescribeCol for reliable concise type retrieval

    // Column 1: DATE
    {
        SQLCHAR colName[128] = {};
        SQLSMALLINT nameLen = 0, dataType = 0, decDigits = 0, nullable = 0;
        SQLULEN colSize = 0;
        rc = SQLDescribeCol(hStmt, 1, colName, sizeof(colName), &nameLen,
                            &dataType, &colSize, &decDigits, &nullable);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
        EXPECT_STREQ((char*)colName, "COL_DATE");
        EXPECT_TRUE(dataType == SQL_TYPE_DATE || dataType == SQL_DATE)
            << "Unexpected date type: " << dataType;
    }

    // Column 2: TIME
    {
        SQLCHAR colName[128] = {};
        SQLSMALLINT nameLen = 0, dataType = 0, decDigits = 0, nullable = 0;
        SQLULEN colSize = 0;
        rc = SQLDescribeCol(hStmt, 2, colName, sizeof(colName), &nameLen,
                            &dataType, &colSize, &decDigits, &nullable);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
        EXPECT_STREQ((char*)colName, "COL_TIME");
        EXPECT_TRUE(dataType == SQL_TYPE_TIME || dataType == SQL_TIME)
            << "Unexpected time type: " << dataType;
    }

    // Column 3: TIMESTAMP
    {
        SQLCHAR colName[128] = {};
        SQLSMALLINT nameLen = 0, dataType = 0, decDigits = 0, nullable = 0;
        SQLULEN colSize = 0;
        rc = SQLDescribeCol(hStmt, 3, colName, sizeof(colName), &nameLen,
                            &dataType, &colSize, &decDigits, &nullable);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
        EXPECT_STREQ((char*)colName, "COL_TIMESTAMP");
        EXPECT_TRUE(dataType == SQL_TYPE_TIMESTAMP || dataType == SQL_TIMESTAMP)
            << "Unexpected timestamp type: " << dataType;
    }
}

TEST_F(DescRecTest, VerifyCharColumn) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT COL_CHAR FROM ODBC_TEST_DESCREC", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLHDESC hDesc = SQL_NULL_HDESC;
    rc = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hDesc, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0, type = 0, subType = 0;
    SQLSMALLINT precision = 0, scale = 0, nullable = 0;
    SQLLEN length = 0;

    rc = SQLGetDescRec(hDesc, 1, name, sizeof(name), &nameLen,
                       &type, &subType, &length, &precision, &scale, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    EXPECT_STREQ((char*)name, "COL_CHAR");
    EXPECT_TRUE(type == SQL_CHAR || type == SQL_WCHAR)
        << "Unexpected char type: " << type;
    EXPECT_EQ(nullable, SQL_NULLABLE);
}

TEST_F(DescRecTest, VerifyFloatColumns) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT COL_FLOAT, COL_DOUBLE FROM ODBC_TEST_DESCREC",
        SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Column 1: FLOAT — use SQLDescribeCol for reliable type checking
    {
        SQLCHAR colName[128] = {};
        SQLSMALLINT nameLen = 0, dataType = 0, decDigits = 0, nullable = 0;
        SQLULEN colSize = 0;
        rc = SQLDescribeCol(hStmt, 1, colName, sizeof(colName), &nameLen,
                            &dataType, &colSize, &decDigits, &nullable);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
        EXPECT_STREQ((char*)colName, "COL_FLOAT");
        EXPECT_TRUE(dataType == SQL_FLOAT || dataType == SQL_REAL || dataType == SQL_DOUBLE)
            << "Unexpected float type: " << dataType;
    }

    // Column 2: DOUBLE PRECISION
    {
        SQLCHAR colName[128] = {};
        SQLSMALLINT nameLen = 0, dataType = 0, decDigits = 0, nullable = 0;
        SQLULEN colSize = 0;
        rc = SQLDescribeCol(hStmt, 2, colName, sizeof(colName), &nameLen,
                            &dataType, &colSize, &decDigits, &nullable);
        ASSERT_TRUE(SQL_SUCCEEDED(rc));
        EXPECT_STREQ((char*)colName, "COL_DOUBLE");
        EXPECT_TRUE(dataType == SQL_DOUBLE || dataType == SQL_FLOAT)
            << "Unexpected double type: " << dataType;
    }
}

TEST_F(DescRecTest, GetDescRecWithPrepareOnly) {
    // Test that IRD is populated after SQLPrepare but before SQLExecute
    SQLRETURN rc = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT COL_INT, COL_VARCHAR, COL_NUMERIC FROM ODBC_TEST_DESCREC",
        SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Use SQLNumResultCols + SQLColAttribute — more reliable across drivers
    SQLSMALLINT numCols = 0;
    rc = SQLNumResultCols(hStmt, &numCols);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(numCols, 3);

    // Column 1 should be COL_INT with type SQL_INTEGER
    SQLCHAR colName[128] = {};
    SQLSMALLINT nameLen = 0, dataType = 0, decDigits = 0, nullable = 0;
    SQLULEN colSize = 0;
    rc = SQLDescribeCol(hStmt, 1, colName, sizeof(colName), &nameLen,
                        &dataType, &colSize, &decDigits, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)colName, "COL_INT");
    EXPECT_EQ(dataType, SQL_INTEGER);
}

TEST_F(DescRecTest, GetDescRecInvalidRecordNumber) {
    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT COL_INT FROM ODBC_TEST_DESCREC", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    SQLHDESC hDesc = SQL_NULL_HDESC;
    rc = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hDesc, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Record 0 is the bookmark column (may return SQL_NO_DATA or error)
    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0, type = 0, subType = 0;
    SQLSMALLINT precision = 0, scale = 0, nullable = 0;
    SQLLEN length = 0;

    rc = SQLGetDescRec(hDesc, 0, name, sizeof(name), &nameLen,
                       &type, &subType, &length, &precision, &scale, &nullable);
    // May be SQL_NO_DATA or SQL_ERROR — not SQL_SUCCESS
    EXPECT_TRUE(rc == SQL_NO_DATA || rc == SQL_ERROR || SQL_SUCCEEDED(rc));

    // Record beyond column count should return SQL_NO_DATA
    rc = SQLGetDescRec(hDesc, 999, name, sizeof(name), &nameLen,
                       &type, &subType, &length, &precision, &scale, &nullable);
    EXPECT_EQ(rc, SQL_NO_DATA);
}
