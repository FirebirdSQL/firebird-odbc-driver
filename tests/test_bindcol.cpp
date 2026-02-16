// tests/test_bindcol.cpp — Dynamic bind/unbind mid-fetch tests
// (Phase 6, ported from psqlodbc bindcol-test)
//
// Tests dynamic unbinding and rebinding of columns while fetching rows.
// Verifies that SQLBindCol(col, NULL) unbinds, SQLFreeStmt(SQL_UNBIND)
// unbinds all, and rebinding mid-fetch works correctly.

#include "test_helpers.h"
#include <cstring>

class BindColTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_BINDCOL",
            "ID INTEGER NOT NULL PRIMARY KEY, LABEL VARCHAR(30)");

        for (int i = 1; i <= 10; i++) {
            ReallocStmt();
            char sql[128];
            snprintf(sql, sizeof(sql),
                "INSERT INTO ODBC_TEST_BINDCOL (ID, LABEL) VALUES (%d, 'foo%d')",
                i, i);
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

// --- Basic bind and fetch ---

TEST_F(BindColTest, BasicBindAndFetch) {
    SQLINTEGER id = 0;
    SQLLEN idInd = 0;
    SQLCHAR label[64] = {};
    SQLLEN labelInd = 0;

    SQLRETURN rc = SQLBindCol(hStmt, 1, SQL_C_LONG, &id, 0, &idInd);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    rc = SQLBindCol(hStmt, 2, SQL_C_CHAR, label, sizeof(label), &labelInd);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID, LABEL FROM ODBC_TEST_BINDCOL ORDER BY ID", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    int rowno = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        rowno++;
        EXPECT_EQ(id, rowno);
        char expected[32];
        snprintf(expected, sizeof(expected), "foo%d", rowno);
        EXPECT_STREQ((char*)label, expected);
    }
    EXPECT_EQ(rowno, 10);
}

// --- Unbind column 2 mid-fetch, then rebind ---
// Mirrors the psqlodbc bindcol-test pattern:
//   rows 1-3: both columns bound
//   rows 4-5: column 2 unbound (NULL pointer)
//   rows 6-7: column 2 rebound
//   row 8: SQL_UNBIND all columns
//   rows 9-10: column 2 rebound

TEST_F(BindColTest, UnbindAndRebindMidFetch) {
    SQLINTEGER id = 0;
    SQLLEN idInd = 0;
    SQLCHAR label[64] = {};
    SQLLEN labelInd = 0;

    SQLRETURN rc = SQLBindCol(hStmt, 1, SQL_C_LONG, &id, 0, &idInd);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    rc = SQLBindCol(hStmt, 2, SQL_C_CHAR, label, sizeof(label), &labelInd);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID, LABEL FROM ODBC_TEST_BINDCOL ORDER BY ID", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    int rowno = 0;
    while (true) {
        rc = SQLFetch(hStmt);
        if (rc == SQL_NO_DATA) break;
        ASSERT_TRUE(SQL_SUCCEEDED(rc))
            << "Fetch failed at row " << (rowno + 1) << ": "
            << GetOdbcError(SQL_HANDLE_STMT, hStmt);

        rowno++;

        if (rowno <= 3) {
            // Both bound — verify both
            EXPECT_EQ(id, rowno);
            char expected[32];
            snprintf(expected, sizeof(expected), "foo%d", rowno);
            EXPECT_STREQ((char*)label, expected) << "Row " << rowno;
        } else if (rowno == 4 || rowno == 5) {
            // Column 2 was unbound — id should still be correct,
            // but label should NOT have been updated
            EXPECT_EQ(id, rowno);
            // label still has the value from row 3 (not updated)
            EXPECT_STREQ((char*)label, "foo3");
        } else if (rowno == 6 || rowno == 7) {
            // Column 2 rebound — both should be correct
            EXPECT_EQ(id, rowno);
            char expected[32];
            snprintf(expected, sizeof(expected), "foo%d", rowno);
            EXPECT_STREQ((char*)label, expected);
        } else if (rowno == 8) {
            // All unbound — neither should be updated.
            // id still has value from row 7
            EXPECT_EQ(id, 7);
            EXPECT_STREQ((char*)label, "foo7");
        } else {
            // Column 2 rebound — label should be correct,
            // id still has value from row 7 (unbound)
            EXPECT_EQ(id, 7);  // still unbound
            char expected[32];
            snprintf(expected, sizeof(expected), "foo%d", rowno);
            EXPECT_STREQ((char*)label, expected);
        }

        // Unbind/rebind at the appropriate rows
        if (rowno == 3) {
            // Unbind column 2
            rc = SQLBindCol(hStmt, 2, SQL_C_CHAR, NULL, 0, NULL);
            ASSERT_TRUE(SQL_SUCCEEDED(rc)) << "Unbind col 2 failed";
        }
        if (rowno == 5) {
            // Rebind column 2
            rc = SQLBindCol(hStmt, 2, SQL_C_CHAR, label, sizeof(label), &labelInd);
            ASSERT_TRUE(SQL_SUCCEEDED(rc)) << "Rebind col 2 failed";
        }
        if (rowno == 7) {
            // Unbind all
            rc = SQLFreeStmt(hStmt, SQL_UNBIND);
            ASSERT_TRUE(SQL_SUCCEEDED(rc)) << "SQL_UNBIND failed";
        }
        if (rowno == 8) {
            // Rebind column 2 only (id stays unbound)
            rc = SQLBindCol(hStmt, 2, SQL_C_CHAR, label, sizeof(label), &labelInd);
            ASSERT_TRUE(SQL_SUCCEEDED(rc)) << "Rebind col 2 after UNBIND failed";
        }
    }
    EXPECT_EQ(rowno, 10);
}

// --- SQLFreeStmt(SQL_UNBIND) then SQLGetData still works ---

TEST_F(BindColTest, UnbindAllThenGetData) {
    SQLINTEGER id = 0;
    SQLLEN idInd = 0;
    SQLCHAR label[64] = {};
    SQLLEN labelInd = 0;

    SQLBindCol(hStmt, 1, SQL_C_LONG, &id, 0, &idInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, label, sizeof(label), &labelInd);

    // Unbind all
    SQLRETURN rc = SQLFreeStmt(hStmt, SQL_UNBIND);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID, LABEL FROM ODBC_TEST_BINDCOL WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // id and label should not have been written to
    EXPECT_EQ(id, 0);
    EXPECT_STREQ((char*)label, "");

    // But SQLGetData should still work
    SQLINTEGER fetchedId = 0;
    SQLLEN fetchedInd = 0;
    rc = SQLGetData(hStmt, 1, SQL_C_SLONG, &fetchedId, 0, &fetchedInd);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(fetchedId, 1);

    SQLCHAR fetchedLabel[64] = {};
    rc = SQLGetData(hStmt, 2, SQL_C_CHAR, fetchedLabel, sizeof(fetchedLabel), &fetchedInd);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)fetchedLabel, "foo1");
}

// --- Bind before exec, then rebind to different type ---

TEST_F(BindColTest, RebindToDifferentType) {
    // Bind column 1 as integer
    SQLINTEGER id = 0;
    SQLLEN idInd = 0;
    SQLRETURN rc = SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_BINDCOL WHERE ID = 5", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(id, 5);

    // Close cursor, rebind same column as string
    SQLCloseCursor(hStmt);
    SQLCHAR strId[32] = {};
    SQLLEN strInd = 0;
    rc = SQLBindCol(hStmt, 1, SQL_C_CHAR, strId, sizeof(strId), &strInd);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID FROM ODBC_TEST_BINDCOL WHERE ID = 7", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)strId, "7");
}

// --- Bind extra column beyond result set width ---

TEST_F(BindColTest, BindBeyondResultSetWidth) {
    SQLINTEGER id = 0;
    SQLLEN idInd = 0;
    SQLCHAR extra[32] = "untouched";
    SQLLEN extraInd = 0;

    // Bind columns 1 and 3 (result set only has 2 columns)
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    SQLBindCol(hStmt, 3, SQL_C_CHAR, extra, sizeof(extra), &extraInd);

    SQLRETURN rc = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT ID, LABEL FROM ODBC_TEST_BINDCOL WHERE ID = 1", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(id, 1);
    // Extra column should not have been modified
    EXPECT_STREQ((char*)extra, "untouched");
}
