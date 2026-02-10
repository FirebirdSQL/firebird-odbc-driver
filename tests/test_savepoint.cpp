// tests/test_savepoint.cpp — Statement-level savepoint isolation tests (M-1, Task 2.3)

#include "test_helpers.h"

class SavepointTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        // Disable auto-commit for savepoint testing
        SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
                                          (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_SVP",
            "ID INTEGER NOT NULL PRIMARY KEY, VAL VARCHAR(30)");
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

TEST_F(SavepointTest, FailedStatementDoesNotCorruptTransaction) {
    // Insert a valid row
    ExecDirect("INSERT INTO ODBC_TEST_SVP (ID, VAL) VALUES (1, 'Good row')");

    // Attempt an insert that violates primary key — this should fail
    ReallocStmt();
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_SVP (ID, VAL) VALUES (1, 'Duplicate')", SQL_NTS);
    EXPECT_EQ(ret, SQL_ERROR) << "Expected PK violation to fail";

    // The key test: the first INSERT should still be intact
    // Without savepoints, Firebird could mark the transaction as doomed
    ReallocStmt();
    ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT VAL FROM ODBC_TEST_SVP WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SELECT after failed INSERT should succeed (savepoint isolation): "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLCHAR val[31] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)val, "Good row");

    // Commit should also succeed
    Commit();
}

TEST_F(SavepointTest, MultipleFailuresDoNotCorruptTransaction) {
    ExecDirect("INSERT INTO ODBC_TEST_SVP (ID, VAL) VALUES (10, 'First')");

    // Multiple failing statements
    for (int i = 0; i < 5; i++) {
        ReallocStmt();
        SQLRETURN ret = SQLExecDirect(hStmt,
            (SQLCHAR*)"INSERT INTO ODBC_TEST_SVP (ID, VAL) VALUES (10, 'Dup')", SQL_NTS);
        EXPECT_EQ(ret, SQL_ERROR);
    }

    // Add another valid row
    ReallocStmt();
    ExecDirect("INSERT INTO ODBC_TEST_SVP (ID, VAL) VALUES (11, 'Second')");

    // Both rows should be visible
    ReallocStmt();
    ExecDirect("SELECT COUNT(*) FROM ODBC_TEST_SVP WHERE ID IN (10, 11)");

    SQLINTEGER count = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(count, 2);

    Commit();
}

TEST_F(SavepointTest, RollbackAfterPartialSuccess) {
    // Insert then rollback — the insert should be gone
    ExecDirect("INSERT INTO ODBC_TEST_SVP (ID, VAL) VALUES (20, 'To be rolled back')");
    Rollback();

    ReallocStmt();
    ExecDirect("SELECT COUNT(*) FROM ODBC_TEST_SVP WHERE ID = 20");

    SQLINTEGER count = -1;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(count, 0);
}

TEST_F(SavepointTest, SuccessfulStatementNotAffectedBySavepointOverhead) {
    // Simple check that the savepoint mechanism doesn't break normal DML
    for (int i = 100; i < 110; i++) {
        ReallocStmt();
        char sql[128];
        snprintf(sql, sizeof(sql),
            "INSERT INTO ODBC_TEST_SVP (ID, VAL) VALUES (%d, 'Row %d')", i, i);
        ExecDirect(sql);
    }
    Commit();

    ReallocStmt();
    ExecDirect("SELECT COUNT(*) FROM ODBC_TEST_SVP WHERE ID >= 100 AND ID < 110");

    SQLINTEGER count = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(SQLFetch(hStmt)));
    EXPECT_EQ(count, 10);
}
