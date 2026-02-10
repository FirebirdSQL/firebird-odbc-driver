// test_server_version.cpp — Tests for server version detection and feature-flagging (Task 4.2)
#include "test_helpers.h"

// ============================================================================
// ServerVersionTest: Verify server version is correctly detected and exposed
// ============================================================================
class ServerVersionTest : public OdbcConnectedTest {};

TEST_F(ServerVersionTest, SQLGetInfoDBMSVer) {
    // SQLGetInfo(SQL_DBMS_VER) should return a non-empty version string
    SQLCHAR version[256] = {};
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_DBMS_VER, version, sizeof(version), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    EXPECT_GT(len, 0) << "DBMS version should be non-empty";
    std::string verStr((char*)version, len);
    // Should contain digits and dots (e.g. "05.00.xxxx ...")
    EXPECT_NE(verStr.find('.'), std::string::npos)
        << "Version string should contain dots: " << verStr;
}

TEST_F(ServerVersionTest, SQLGetInfoDBMSName) {
    // SQLGetInfo(SQL_DBMS_NAME) should return "Firebird"
    SQLCHAR name[256] = {};
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_DBMS_NAME, name, sizeof(name), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    std::string nameStr((char*)name, len);
    EXPECT_NE(nameStr.find("Firebird"), std::string::npos)
        << "DBMS name should contain 'Firebird': " << nameStr;
}

TEST_F(ServerVersionTest, EngineVersionFromSQL) {
    // Query the engine version directly to cross-check
    ExecDirect("SELECT rdb$get_context('SYSTEM','ENGINE_VERSION') FROM rdb$database");
    SQLCHAR version[256] = {};
    SQLLEN ind = 0;
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLGetData(hStmt, 1, SQL_C_CHAR, version, sizeof(version), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    std::string verStr((char*)version);
    // Should be like "5.0.1" or "4.0.5"
    EXPECT_GE(verStr.length(), 5u) << "Engine version too short: " << verStr;
    // First char should be a digit >= 3
    EXPECT_GE(verStr[0], '3') << "Expected Firebird 3.0+: " << verStr;
}

TEST_F(ServerVersionTest, SQLGetTypeInfoShowsAllBaseTypes) {
    // SQLGetTypeInfo(SQL_ALL_TYPES) should return at least the 22 base types
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    int count = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt)))
        ++count;

    // At least 22 base types (CHAR, VARCHAR, BLOB text, WCHAR, WVARCHAR, WBLOB,
    // BLOB binary x3, NUMERIC, DECIMAL, INTEGER, TINYINT, SMALLINT, FLOAT, REAL,
    // DOUBLE, BIGINT, BOOLEAN, DATE, TIME, TIMESTAMP)
    // Plus 4 FB4+ types (INT128, DECFLOAT, TIME WITH TZ, TIMESTAMP WITH TZ) on FB5
    EXPECT_GE(count, 22) << "Expected at least 22 base type entries";
}

TEST_F(ServerVersionTest, SQLGetTypeInfoShowsFB4TypesOnFB5) {
    // On Firebird 5.0, SQLGetTypeInfo should also include FB4+ types
    // First check if we're on FB 4.0+
    SQLCHAR version[256] = {};
    SQLSMALLINT len = 0;
    SQLGetInfo(hDbc, SQL_DBMS_VER, version, sizeof(version), &len);
    std::string verStr((char*)version, len);

    // Parse major version from the version string (format: "MM.mm.bbbb ...")
    int major = 0;
    if (verStr.length() >= 2)
        major = std::stoi(verStr.substr(0, 2));

    if (major < 4) {
        GTEST_SKIP() << "Test requires Firebird 4.0+ (current: " << verStr << ")";
    }

    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    bool foundInt128 = false;
    bool foundDecfloat = false;
    bool foundTimeTZ = false;
    bool foundTimestampTZ = false;

    SQLCHAR typeName[256] = {};
    SQLLEN typeNameInd = 0;

    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        ret = SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &typeNameInd);
        if (SQL_SUCCEEDED(ret) && typeNameInd > 0) {
            std::string name((char*)typeName, typeNameInd);
            if (name == "INT128") foundInt128 = true;
            if (name == "DECFLOAT") foundDecfloat = true;
            if (name == "TIME WITH TIME ZONE") foundTimeTZ = true;
            if (name == "TIMESTAMP WITH TIME ZONE") foundTimestampTZ = true;
        }
    }

    EXPECT_TRUE(foundInt128) << "INT128 type should be listed on FB4+";
    EXPECT_TRUE(foundDecfloat) << "DECFLOAT type should be listed on FB4+";
    EXPECT_TRUE(foundTimeTZ) << "TIME WITH TIME ZONE should be listed on FB4+";
    EXPECT_TRUE(foundTimestampTZ) << "TIMESTAMP WITH TIME ZONE should be listed on FB4+";
}

TEST_F(ServerVersionTest, ScrollOptionsReported) {
    // SQLGetInfo should report scroll options
    SQLUINTEGER scrollOpts = 0;
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_SCROLL_OPTIONS, &scrollOpts, sizeof(scrollOpts), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    EXPECT_TRUE(scrollOpts & SQL_SO_FORWARD_ONLY) << "Should support forward-only";
    EXPECT_TRUE(scrollOpts & SQL_SO_STATIC) << "Should support static scrollable cursors";
}
