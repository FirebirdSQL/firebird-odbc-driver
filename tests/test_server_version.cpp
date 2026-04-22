// test_server_version.cpp — Tests for server version detection and feature-flagging (Task 4.2)
#include "test_helpers.h"
#include <cstdio>
#include <string>

// ============================================================================
// ServerVersionTest: Verify server version is correctly detected and exposed
// ============================================================================
class ServerVersionTest : public OdbcConnectedTest {
protected:
    // Query the Firebird engine version string (e.g. "5.0.3" / "6.0.0" / "3.0.12")
    // straight from the server via rdb$get_context — this is the ground truth we
    // compare SQL_DBMS_VER against below.
    std::string GetEngineVersionFromServer() {
        SQLRETURN ret = SQLExecDirect(hStmt,
            (SQLCHAR*)"SELECT rdb$get_context('SYSTEM','ENGINE_VERSION') FROM rdb$database",
            SQL_NTS);
        EXPECT_TRUE(SQL_SUCCEEDED(ret));
        ret = SQLFetch(hStmt);
        EXPECT_TRUE(SQL_SUCCEEDED(ret));
        SQLCHAR buf[64] = {};
        SQLLEN ind = 0;
        ret = SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
        EXPECT_TRUE(SQL_SUCCEEDED(ret));
        SQLCloseCursor(hStmt);
        return std::string((char*)buf);
    }
};

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
    std::string verStr = GetEngineVersionFromServer();
    // Should be like "5.0.1" or "4.0.5"
    EXPECT_GE(verStr.length(), 5u) << "Engine version too short: " << verStr;
    // First char should be a digit >= 3
    EXPECT_GE(verStr[0], '3') << "Expected Firebird 3.0+: " << verStr;
}

// ----------------------------------------------------------------------------
// Regression tests for the Firebird-product-version-vs-engine-implementation-
// version bug: until #292 + follow-up, SQL_DBMS_VER was built from
// isc_info_version (which reports the ENGINE / ODS-compat implementation
// number, currently "6.3.x" on every supported Firebird) instead of from
// isc_info_firebird_version (the actual product version, e.g. "5.0.3").  On
// Firebird 5.0.3 SQLGetInfo(SQL_DBMS_VER) returned "06.03.1683 WI-V Firebird 5.0"
// — misidentifying every server as Firebird 6.x to any downstream ODBC tool
// or version-gated test (see SKIP_ON_FIREBIRD6 in test_helpers.h, which was
// silently firing on FB 3 / 4 / 5 too).
//
// These tests cross-reference SQL_DBMS_VER against rdb$get_context(ENGINE_VERSION)
// and against SQL_DBMS_NAME.  The pre-fix behaviour fails all three of them.
// ----------------------------------------------------------------------------

TEST_F(ServerVersionTest, DBMSVerMajorMatchesEngineVersion) {
    // SQL_DBMS_VER per ODBC spec is a "##.##.####" prefix followed by an
    // implementation-defined suffix.  The first two digits are the product
    // major version — they must match the first token of the engine version
    // reported by rdb$get_context.
    SQLCHAR dbmsVer[256] = {};
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_DBMS_VER, dbmsVer, sizeof(dbmsVer), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    std::string dbmsVerStr((char*)dbmsVer, len);

    std::string engineVer = GetEngineVersionFromServer();
    ASSERT_FALSE(engineVer.empty());

    // Parse "MM" prefix of SQL_DBMS_VER (ODBC mandates the format; leading
    // whitespace is not allowed).
    ASSERT_GE(dbmsVerStr.size(), 2u);
    const int dbmsMajor = std::atoi(dbmsVerStr.substr(0, 2).c_str());

    // Parse first token of engine version (split at '.').
    const int engineMajor = std::atoi(engineVer.c_str());

    EXPECT_EQ(dbmsMajor, engineMajor)
        << "SQL_DBMS_VER major must match rdb$get_context('ENGINE_VERSION') major.\n"
        << "  SQL_DBMS_VER:     '" << dbmsVerStr << "'\n"
        << "  ENGINE_VERSION:   '" << engineVer << "'\n"
        << "  Parsed DBMS_VER major: " << dbmsMajor << "\n"
        << "  Parsed engine major:   " << engineMajor;
}

TEST_F(ServerVersionTest, DBMSVerMinorMatchesEngineVersion) {
    SQLCHAR dbmsVer[256] = {};
    SQLSMALLINT len = 0;
    SQLRETURN ret = SQLGetInfo(hDbc, SQL_DBMS_VER, dbmsVer, sizeof(dbmsVer), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    std::string dbmsVerStr((char*)dbmsVer, len);

    std::string engineVer = GetEngineVersionFromServer();
    ASSERT_FALSE(engineVer.empty());

    // Parse "MM.mm" minor (bytes 3-4 of SQL_DBMS_VER).
    ASSERT_GE(dbmsVerStr.size(), 5u);
    const int dbmsMinor = std::atoi(dbmsVerStr.substr(3, 2).c_str());

    // Parse second token of engine version.
    auto firstDot = engineVer.find('.');
    ASSERT_NE(firstDot, std::string::npos);
    const int engineMinor = std::atoi(engineVer.substr(firstDot + 1).c_str());

    EXPECT_EQ(dbmsMinor, engineMinor)
        << "SQL_DBMS_VER minor must match rdb$get_context('ENGINE_VERSION') minor.\n"
        << "  SQL_DBMS_VER:     '" << dbmsVerStr << "'\n"
        << "  ENGINE_VERSION:   '" << engineVer << "'";
}

TEST_F(ServerVersionTest, DBMSVerSuffixContainsDBMSName) {
    // Beyond the "##.##.####" prefix the string is implementation-defined, but
    // the driver has always appended " <impl-prefix> <product name>" where the
    // product name contains the DBMS_NAME.  If that invariant ever slips we
    // want to know about it — it's the quickest smoke test that the whole
    // string wasn't clobbered.
    SQLCHAR dbmsVer[256] = {}, dbmsName[256] = {};
    SQLSMALLINT verLen = 0, nameLen = 0;
    ASSERT_TRUE(SQL_SUCCEEDED(
        SQLGetInfo(hDbc, SQL_DBMS_VER,  dbmsVer,  sizeof(dbmsVer),  &verLen)));
    ASSERT_TRUE(SQL_SUCCEEDED(
        SQLGetInfo(hDbc, SQL_DBMS_NAME, dbmsName, sizeof(dbmsName), &nameLen)));
    std::string verStr((char*)dbmsVer, verLen);
    std::string nameStr((char*)dbmsName, nameLen);

    EXPECT_NE(verStr.find(nameStr), std::string::npos)
        << "SQL_DBMS_VER suffix should include the DBMS_NAME.\n"
        << "  SQL_DBMS_VER:   '" << verStr << "'\n"
        << "  SQL_DBMS_NAME:  '" << nameStr << "'";
}

TEST_F(ServerVersionTest, SQLGetTypeInfoShowsAllBaseTypes) {
    GTEST_SKIP() << "Requires Phase 4: server version detection for type count";
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
    GTEST_SKIP() << "Requires Phase 4: server version detection for FB4+ type info";
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
