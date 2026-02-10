// test_odbc38_compliance.cpp
// Tests for ODBC 3.8 compliance features:
// - SQL_OV_ODBC3_80 env attr support
// - SQL_DRIVER_ODBC_VER = "03.80"
// - SQL_ATTR_RESET_CONNECTION
// - SQL_GD_OUTPUT_PARAMS in SQL_GETDATA_EXTENSIONS
// - SQL_ASYNC_DBC_FUNCTIONS info type

#include "test_helpers.h"

// ============================================================
// Tests that don't require a database connection
// ============================================================

class Odbc38EnvTest : public ::testing::Test {
public:
    SQLHENV hEnv = SQL_NULL_HENV;

    void SetUp() override {
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Failed to allocate ENV handle";
    }

    void TearDown() override {
        if (hEnv != SQL_NULL_HENV) {
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        }
    }
};

// Test: SQL_OV_ODBC3_80 is accepted as a valid ODBC version
TEST_F(Odbc38EnvTest, AcceptsOdbcVersion380) {
    SQLRETURN ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION,
                                  (SQLPOINTER)SQL_OV_ODBC3_80, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQL_OV_ODBC3_80 should be accepted: " << GetOdbcError(SQL_HANDLE_ENV, hEnv);
}

// Test: After setting SQL_OV_ODBC3_80, retrieving it returns 380
TEST_F(Odbc38EnvTest, GetOdbcVersion380) {
    SQLRETURN ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION,
                                  (SQLPOINTER)SQL_OV_ODBC3_80, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER version = 0;
    ret = SQLGetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, &version, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(version, (SQLINTEGER)SQL_OV_ODBC3_80);
}

// Test: SQL_OV_ODBC2 is still accepted
TEST_F(Odbc38EnvTest, AcceptsOdbcVersion2) {
    SQLRETURN ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION,
                                  (SQLPOINTER)SQL_OV_ODBC2, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
}

// Test: SQL_OV_ODBC3 is still accepted
TEST_F(Odbc38EnvTest, AcceptsOdbcVersion3) {
    SQLRETURN ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION,
                                  (SQLPOINTER)SQL_OV_ODBC3, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
}

// Test: Invalid ODBC version is rejected
TEST_F(Odbc38EnvTest, RejectsInvalidOdbcVersion) {
    SQLRETURN ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION,
                                  (SQLPOINTER)(intptr_t)999, 0);
    EXPECT_EQ(ret, SQL_ERROR);
}

// Test: Can allocate connection handle after setting SQL_OV_ODBC3_80
TEST_F(Odbc38EnvTest, AllocConnectionAfter380) {
    SQLRETURN ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION,
                                  (SQLPOINTER)SQL_OV_ODBC3_80, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLHDBC hDbc = SQL_NULL_HDBC;
    ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Should allocate DBC after SQL_OV_ODBC3_80";
    SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
}

// ============================================================
// Tests that require a database connection
// ============================================================

class Odbc38ConnectedTest : public OdbcConnectedTest {};

// Test: SQL_DRIVER_ODBC_VER returns "03.80"
TEST_F(Odbc38ConnectedTest, DriverOdbcVerIs380) {
    REQUIRE_FIREBIRD_CONNECTION();

    SQLCHAR version[32] = {};
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_DRIVER_ODBC_VER, version, sizeof(version), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    EXPECT_STREQ((char*)version, "03.80") << "Driver should report ODBC 3.80 compliance";
}

// Test: SQL_GETDATA_EXTENSIONS includes SQL_GD_OUTPUT_PARAMS
TEST_F(Odbc38ConnectedTest, GetDataExtensionsIncludesOutputParams) {
    REQUIRE_FIREBIRD_CONNECTION();

    SQLUINTEGER extensions = 0;
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_GETDATA_EXTENSIONS, &extensions, sizeof(extensions), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DBC, hDbc);

    EXPECT_TRUE(extensions & SQL_GD_ANY_COLUMN) << "Should support SQL_GD_ANY_COLUMN";
    EXPECT_TRUE(extensions & SQL_GD_ANY_ORDER) << "Should support SQL_GD_ANY_ORDER";
    EXPECT_TRUE(extensions & SQL_GD_BLOCK) << "Should support SQL_GD_BLOCK";
    EXPECT_TRUE(extensions & SQL_GD_BOUND) << "Should support SQL_GD_BOUND";
    EXPECT_TRUE(extensions & SQL_GD_OUTPUT_PARAMS) << "Should support SQL_GD_OUTPUT_PARAMS for ODBC 3.8";
}

// Test: SQL_ASYNC_DBC_FUNCTIONS reports not capable
TEST_F(Odbc38ConnectedTest, AsyncDbcFunctionsReportsNotCapable) {
    REQUIRE_FIREBIRD_CONNECTION();

    SQLUINTEGER value = 0xFFFF;
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_ASYNC_DBC_FUNCTIONS, &value, sizeof(value), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    EXPECT_EQ(value, (SQLUINTEGER)SQL_ASYNC_DBC_NOT_CAPABLE)
        << "Driver should report SQL_ASYNC_DBC_NOT_CAPABLE";
}

// Test: SQL_ATTR_RESET_CONNECTION is accepted for connection pool reset
TEST_F(Odbc38ConnectedTest, ResetConnectionAccepted) {
    REQUIRE_FIREBIRD_CONNECTION();

    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                                      (SQLPOINTER)(intptr_t)SQL_RESET_CONNECTION_YES,
                                      SQL_IS_UINTEGER);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQL_ATTR_RESET_CONNECTION should be accepted: "
        << GetOdbcError(SQL_HANDLE_DBC, hDbc);
}

// Test: After reset connection, autocommit is restored to ON
TEST_F(Odbc38ConnectedTest, ResetConnectionRestoresAutocommit) {
    REQUIRE_FIREBIRD_CONNECTION();

    // Turn off autocommit
    SQLRETURN ret = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)(intptr_t)SQL_AUTOCOMMIT_OFF,
                                      SQL_IS_UINTEGER);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Reset connection
    ret = SQLSetConnectAttr(hDbc, SQL_ATTR_RESET_CONNECTION,
                            (SQLPOINTER)(intptr_t)SQL_RESET_CONNECTION_YES,
                            SQL_IS_UINTEGER);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Check autocommit is back to ON
    SQLUINTEGER autocommit = 0;
    ret = SQLGetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, &autocommit, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(autocommit, (SQLUINTEGER)SQL_AUTOCOMMIT_ON)
        << "Autocommit should be restored to ON after reset";
}

// Test: ODBC 3.8 ODBC_INTERFACE_CONFORMANCE reported correctly
TEST_F(Odbc38ConnectedTest, OdbcInterfaceConformance) {
    REQUIRE_FIREBIRD_CONNECTION();

    SQLUINTEGER conformance = 0;
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_ODBC_INTERFACE_CONFORMANCE, &conformance, sizeof(conformance), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    // Should be at least Level 1
    EXPECT_GE(conformance, (SQLUINTEGER)SQL_OIC_LEVEL1);
}
