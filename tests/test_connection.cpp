#include "test_helpers.h"

// Basic connection and CRUD tests using the standard OdbcConnectedTest fixture
class FirebirdODBCTest : public OdbcConnectedTest {};

TEST_F(FirebirdODBCTest, ExecuteSimpleQuery) {
    ExecDirect("SELECT CURRENT_TIMESTAMP FROM RDB$DATABASE");
    SQLRETURN ret = SQLFetch(hStmt);
    EXPECT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to fetch result";
}

TEST_F(FirebirdODBCTest, CreateAndDropTable) {
    TempTable table(this, "ODBC_TEST_TABLE", "ID INTEGER, NAME VARCHAR(50)");
    // TempTable RAII handles CREATE and DROP — just verify we got here
    SUCCEED();
}

TEST_F(FirebirdODBCTest, InsertAndRetrieveData) {
    TempTable table(this, "ODBC_TEST_DATA", "ID INTEGER, NAME VARCHAR(50)");

    ExecDirect("INSERT INTO ODBC_TEST_DATA (ID, NAME) VALUES (1, 'Test Name')");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT ID, NAME FROM ODBC_TEST_DATA");

    SQLINTEGER id = 0;
    SQLCHAR name[51] = {};
    SQLLEN idInd = 0, nameInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, name, sizeof(name), &nameInd);

    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to fetch data";

    EXPECT_EQ(id, 1);
    EXPECT_STREQ((char*)name, "Test Name");
}
