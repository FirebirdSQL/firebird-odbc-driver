#include "test_helpers.h"

// Test fixture for ODBC connection tests
class FirebirdODBCTest : public ::testing::Test {
protected:
    SQLHENV hEnv;
    SQLHDBC hDbc;
    SQLHSTMT hStmt;
    
    void SetUp() override {
        hEnv = SQL_NULL_HENV;
        hDbc = SQL_NULL_HDBC;
        hStmt = SQL_NULL_HSTMT;
    }
    
    void TearDown() override {
        if (hStmt != SQL_NULL_HSTMT) {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        if (hDbc != SQL_NULL_HDBC) {
            SQLDisconnect(hDbc);
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        }
        if (hEnv != SQL_NULL_HENV) {
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        }
    }
    
    bool AllocateHandles() {
        SQLRETURN ret;
        
        // Allocate environment handle
        ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
        if (!SQL_SUCCEEDED(ret)) {
            return false;
        }
        
        // Set ODBC version
        ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        if (!SQL_SUCCEEDED(ret)) {
            return false;
        }
        
        // Allocate connection handle
        ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
        if (!SQL_SUCCEEDED(ret)) {
            return false;
        }
        
        return true;
    }
    
    std::string GetErrorMessage(SQLSMALLINT handleType, SQLHANDLE handle) {
        SQLCHAR sqlState[6];
        SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER nativeError;
        SQLSMALLINT messageLength;
        
        SQLRETURN ret = SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeError,
                                       message, sizeof(message), &messageLength);
        
        if (SQL_SUCCEEDED(ret)) {
            return std::string("SQLSTATE: ") + (char*)sqlState + 
                   ", Message: " + (char*)message;
        }
        return "Unable to retrieve error message";
    }
};

// Test: Check if connection string is provided
TEST_F(FirebirdODBCTest, ConnectionStringProvided) {
    std::string connStr = GetConnectionString();
    if (connStr.empty()) {
        GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION environment variable not set";
    }
}

// Test: Allocate ODBC handles
TEST_F(FirebirdODBCTest, AllocateODBCHandles) {
    EXPECT_TRUE(AllocateHandles()) << "Failed to allocate ODBC handles";
}

// Test: Connect to Firebird database
TEST_F(FirebirdODBCTest, ConnectToDatabase) {
    std::string connStr = GetConnectionString();
    if (connStr.empty()) {
        GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION environment variable not set";
    }
    
    ASSERT_TRUE(AllocateHandles()) << "Failed to allocate ODBC handles";
    
    // Connect using connection string
    SQLCHAR outConnStr[1024];
    SQLSMALLINT outConnStrLen;
    
    SQLRETURN ret = SQLDriverConnect(hDbc, NULL, 
                                      (SQLCHAR*)connStr.c_str(), SQL_NTS,
                                      outConnStr, sizeof(outConnStr), &outConnStrLen,
                                      SQL_DRIVER_NOPROMPT);
    
    if (!SQL_SUCCEEDED(ret)) {
        std::string errorMsg = GetErrorMessage(SQL_HANDLE_DBC, hDbc);
        FAIL() << "Failed to connect to database: " << errorMsg;
    }
    
    SUCCEED();
}

// Test: Execute a simple query
TEST_F(FirebirdODBCTest, ExecuteSimpleQuery) {
    std::string connStr = GetConnectionString();
    if (connStr.empty()) {
        GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION environment variable not set";
    }
    
    ASSERT_TRUE(AllocateHandles()) << "Failed to allocate ODBC handles";
    
    // Connect
    SQLCHAR outConnStr[1024];
    SQLSMALLINT outConnStrLen;
    SQLRETURN ret = SQLDriverConnect(hDbc, NULL, 
                                      (SQLCHAR*)connStr.c_str(), SQL_NTS,
                                      outConnStr, sizeof(outConnStr), &outConnStrLen,
                                      SQL_DRIVER_NOPROMPT);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to connect: " << GetErrorMessage(SQL_HANDLE_DBC, hDbc);
    
    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate statement handle";
    
    // Execute a simple query (get current timestamp from RDB$DATABASE)
    const char* query = "SELECT CURRENT_TIMESTAMP FROM RDB$DATABASE";
    ret = SQLExecDirect(hStmt, (SQLCHAR*)query, SQL_NTS);
    
    if (!SQL_SUCCEEDED(ret)) {
        std::string errorMsg = GetErrorMessage(SQL_HANDLE_STMT, hStmt);
        FAIL() << "Failed to execute query: " << errorMsg;
    }
    
    // Fetch result
    ret = SQLFetch(hStmt);
    EXPECT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to fetch result";
    
    SUCCEED();
}

// Test: Create and drop a test table
TEST_F(FirebirdODBCTest, CreateAndDropTable) {
    std::string connStr = GetConnectionString();
    if (connStr.empty()) {
        GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION environment variable not set";
    }
    
    ASSERT_TRUE(AllocateHandles()) << "Failed to allocate ODBC handles";
    
    // Connect
    SQLCHAR outConnStr[1024];
    SQLSMALLINT outConnStrLen;
    SQLRETURN ret = SQLDriverConnect(hDbc, NULL, 
                                      (SQLCHAR*)connStr.c_str(), SQL_NTS,
                                      outConnStr, sizeof(outConnStr), &outConnStrLen,
                                      SQL_DRIVER_NOPROMPT);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to connect: " << GetErrorMessage(SQL_HANDLE_DBC, hDbc);
    
    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate statement handle";
    
    // Drop table if exists (may fail, that's okay)
    SQLExecDirect(hStmt, (SQLCHAR*)"DROP TABLE ODBC_TEST_TABLE", SQL_NTS);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    
    // Allocate new statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate statement handle";
    
    // Create table
    const char* createQuery = "CREATE TABLE ODBC_TEST_TABLE (ID INTEGER, NAME VARCHAR(50))";
    ret = SQLExecDirect(hStmt, (SQLCHAR*)createQuery, SQL_NTS);
    
    if (!SQL_SUCCEEDED(ret)) {
        std::string errorMsg = GetErrorMessage(SQL_HANDLE_STMT, hStmt);
        FAIL() << "Failed to create table: " << errorMsg;
    }
    
    // Commit
    ret = SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    EXPECT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to commit transaction";
    
    // Drop table
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate statement handle";
    
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"DROP TABLE ODBC_TEST_TABLE", SQL_NTS);
    EXPECT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to drop table: " << GetErrorMessage(SQL_HANDLE_STMT, hStmt);
    
    // Commit
    ret = SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    EXPECT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to commit transaction";
    
    SUCCEED();
}

// Test: Insert and retrieve data
TEST_F(FirebirdODBCTest, InsertAndRetrieveData) {
    std::string connStr = GetConnectionString();
    if (connStr.empty()) {
        GTEST_SKIP() << "FIREBIRD_ODBC_CONNECTION environment variable not set";
    }
    
    ASSERT_TRUE(AllocateHandles()) << "Failed to allocate ODBC handles";
    
    // Connect
    SQLCHAR outConnStr[1024];
    SQLSMALLINT outConnStrLen;
    SQLRETURN ret = SQLDriverConnect(hDbc, NULL, 
                                      (SQLCHAR*)connStr.c_str(), SQL_NTS,
                                      outConnStr, sizeof(outConnStr), &outConnStrLen,
                                      SQL_DRIVER_NOPROMPT);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to connect: " << GetErrorMessage(SQL_HANDLE_DBC, hDbc);
    
    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate statement handle";
    
    // Drop table if exists (may fail, that's okay)
    SQLExecDirect(hStmt, (SQLCHAR*)"DROP TABLE ODBC_TEST_DATA", SQL_NTS);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    
    // Create table
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"CREATE TABLE ODBC_TEST_DATA (ID INTEGER, NAME VARCHAR(50))", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to create table: " << GetErrorMessage(SQL_HANDLE_STMT, hStmt);
    
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    
    // Insert data
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO ODBC_TEST_DATA (ID, NAME) VALUES (1, 'Test Name')", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to insert data: " << GetErrorMessage(SQL_HANDLE_STMT, hStmt);
    
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    
    // Retrieve data
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT ID, NAME FROM ODBC_TEST_DATA", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to select data: " << GetErrorMessage(SQL_HANDLE_STMT, hStmt);
    
    SQLINTEGER id;
    SQLCHAR name[51];
    SQLLEN idInd, nameInd;
    
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, 0, &idInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, name, sizeof(name), &nameInd);
    
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to fetch data";
    
    EXPECT_EQ(id, 1);
    EXPECT_STREQ((char*)name, "Test Name");
    
    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLExecDirect(hStmt, (SQLCHAR*)"DROP TABLE ODBC_TEST_DATA", SQL_NTS);
    SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
    
    SUCCEED();
}
