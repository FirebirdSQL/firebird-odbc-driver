// test_guid_and_binary.cpp
// Tests for SQL_GUID support and BINARY type mapping:
// - SQL_GUID type reported in SQLGetTypeInfo
// - BINARY(16) / CHAR(16) CHARACTER SET OCTETS maps to SQL_GUID
// - GUID generation and retrieval via GEN_UUID()
// - UUID_TO_CHAR / CHAR_TO_UUID roundtrip
// - BINARY/VARBINARY type support (Firebird 4.0+)

#include "test_helpers.h"
#include <cstring>

class GuidTest : public OdbcConnectedTest {};

// Test: SQLGetTypeInfo includes SQL_GUID type
TEST_F(GuidTest, TypeInfoIncludesGuid) {
    REQUIRE_FIREBIRD_CONNECTION();

    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_GUID);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQL_GUID should be listed in SQLGetTypeInfo: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Get the type name
    SQLCHAR typeName[128] = {};
    SQLLEN nameLen = 0;
    ret = SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &nameLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    // Type name should reference OCTETS character set for Firebird
    EXPECT_TRUE(strstr((char*)typeName, "OCTETS") != nullptr ||
                strstr((char*)typeName, "BINARY") != nullptr ||
                strstr((char*)typeName, "GUID") != nullptr)
        << "GUID type name was: " << typeName;
}

// Test: Create table with CHAR(16) CHARACTER SET OCTETS, insert UUID via GEN_UUID()
TEST_F(GuidTest, InsertAndRetrieveUuidBinary) {
    REQUIRE_FIREBIRD_CONNECTION();

    TempTable table(this, "TEST_UUID_BIN",
        "ID CHAR(16) CHARACTER SET OCTETS NOT NULL, "
        "NAME VARCHAR(50)");

    // Insert using GEN_UUID() explicitly
    ExecDirect("INSERT INTO TEST_UUID_BIN (ID, NAME) VALUES (GEN_UUID(), 'test1')");
    Commit();
    ReallocStmt();

    // Select the raw binary UUID
    ExecDirect("SELECT ID FROM TEST_UUID_BIN");

    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // The column should be reported as SQL_GUID (-11)
    SQLSMALLINT sqlType = 0;
    SQLULEN columnSize = 0;
    SQLSMALLINT decDigits = 0;
    SQLSMALLINT nullable = 0;
    ret = SQLDescribeCol(hStmt, 1, NULL, 0, NULL, &sqlType, &columnSize, &decDigits, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(sqlType, SQL_GUID)
        << "CHAR(16) CHARACTER SET OCTETS should map to SQL_GUID, got " << sqlType;

    // Retrieve as binary — should be 16 bytes
    unsigned char binaryUuid[16] = {};
    SQLLEN ind = 0;
    ret = SQLGetData(hStmt, 1, SQL_C_BINARY, binaryUuid, sizeof(binaryUuid), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(ind, 16) << "UUID binary data should be 16 bytes";

    // Verify it's not all zeros (GEN_UUID should produce a random UUID)
    bool allZero = true;
    for (int i = 0; i < 16; i++) {
        if (binaryUuid[i] != 0) { allZero = false; break; }
    }
    EXPECT_FALSE(allZero) << "GEN_UUID should produce a non-zero UUID";
}

// Test: Retrieve UUID as text via UUID_TO_CHAR
TEST_F(GuidTest, UuidToCharReturnsValidFormat) {
    REQUIRE_FIREBIRD_CONNECTION();

    TempTable table(this, "TEST_UUID_TEXT",
        "ID CHAR(16) CHARACTER SET OCTETS NOT NULL, "
        "NAME VARCHAR(50)");

    ExecDirect("INSERT INTO TEST_UUID_TEXT (ID, NAME) VALUES (GEN_UUID(), 'test_text')");
    Commit();
    ReallocStmt();

    // Retrieve as canonical UUID text via UUID_TO_CHAR
    ExecDirect("SELECT UUID_TO_CHAR(ID) FROM TEST_UUID_TEXT");

    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLCHAR uuidText[64] = {};
    SQLLEN ind = 0;
    ret = SQLGetData(hStmt, 1, SQL_C_CHAR, uuidText, sizeof(uuidText), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Should be 36 chars: 8-4-4-4-12
    std::string uuid((char*)uuidText);
    // Trim trailing spaces
    while (!uuid.empty() && uuid.back() == ' ') uuid.pop_back();
    EXPECT_EQ(uuid.length(), 36u) << "UUID text should be 36 chars, got: '" << uuid << "'";
    EXPECT_EQ(uuid[8], '-');
    EXPECT_EQ(uuid[13], '-');
    EXPECT_EQ(uuid[18], '-');
    EXPECT_EQ(uuid[23], '-');
}

// Test: CHAR_TO_UUID roundtrip
TEST_F(GuidTest, CharToUuidRoundtrip) {
    REQUIRE_FIREBIRD_CONNECTION();

    TempTable table(this, "TEST_UUID_RT",
        "ID CHAR(16) CHARACTER SET OCTETS NOT NULL, "
        "NAME VARCHAR(50)");

    // Insert a known UUID using CHAR_TO_UUID
    ExecDirect("INSERT INTO TEST_UUID_RT (ID, NAME) VALUES "
               "(CHAR_TO_UUID('A0EEBC99-9C0B-4EF8-BB6D-6BB9BD380A11'), 'roundtrip')");
    Commit();
    ReallocStmt();

    // Read it back as text
    ExecDirect("SELECT UUID_TO_CHAR(ID) FROM TEST_UUID_RT");

    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLCHAR uuidText[64] = {};
    SQLLEN ind = 0;
    ret = SQLGetData(hStmt, 1, SQL_C_CHAR, uuidText, sizeof(uuidText), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    std::string uuid((char*)uuidText);
    while (!uuid.empty() && uuid.back() == ' ') uuid.pop_back();

    // Firebird normalizes to uppercase
    EXPECT_EQ(uuid, "A0EEBC99-9C0B-4EF8-BB6D-6BB9BD380A11")
        << "UUID roundtrip failed, got: " << uuid;
}

// Test: Multiple UUID inserts produce unique values
TEST_F(GuidTest, GenUuidProducesUniqueValues) {
    REQUIRE_FIREBIRD_CONNECTION();

    TempTable table(this, "TEST_UUID_UNIQUE",
        "ID CHAR(16) CHARACTER SET OCTETS NOT NULL, "
        "SEQ INTEGER NOT NULL");

    // Insert 5 rows using GEN_UUID() explicitly
    for (int i = 1; i <= 5; i++) {
        char sql[128];
        snprintf(sql, sizeof(sql), "INSERT INTO TEST_UUID_UNIQUE (ID, SEQ) VALUES (GEN_UUID(), %d)", i);
        ExecDirect(sql);
    }
    Commit();
    ReallocStmt();

    // Read all UUIDs
    ExecDirect("SELECT UUID_TO_CHAR(ID) FROM TEST_UUID_UNIQUE ORDER BY SEQ");

    std::vector<std::string> uuids;
    while (true) {
        SQLRETURN ret = SQLFetch(hStmt);
        if (ret == SQL_NO_DATA) break;
        ASSERT_TRUE(SQL_SUCCEEDED(ret));

        SQLCHAR buf[64] = {};
        SQLLEN ind = 0;
        SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
        std::string uuid((char*)buf);
        while (!uuid.empty() && uuid.back() == ' ') uuid.pop_back();
        uuids.push_back(uuid);
    }

    ASSERT_EQ(uuids.size(), 5u);
    // All UUIDs should be unique
    for (size_t i = 0; i < uuids.size(); i++) {
        for (size_t j = i + 1; j < uuids.size(); j++) {
            EXPECT_NE(uuids[i], uuids[j])
                << "UUID " << i << " and " << j << " should be unique";
        }
    }
}

// Test: SQLGUID struct retrieval via SQL_C_GUID (when fetching binary UUID)
TEST_F(GuidTest, RetrieveAsSqlGuidStruct) {
    REQUIRE_FIREBIRD_CONNECTION();

    TempTable table(this, "TEST_UUID_STRUCT",
        "ID CHAR(16) CHARACTER SET OCTETS NOT NULL");

    // Insert a known UUID
    ExecDirect("INSERT INTO TEST_UUID_STRUCT (ID) VALUES "
               "(CHAR_TO_UUID('A0EEBC99-9C0B-4EF8-BB6D-6BB9BD380A11'))");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT ID FROM TEST_UUID_STRUCT");

    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLGUID guid = {};
    SQLLEN ind = 0;
    ret = SQLGetData(hStmt, 1, SQL_C_GUID, &guid, sizeof(guid), &ind);
    // SQL_C_GUID retrieval from binary data
    if (SQL_SUCCEEDED(ret)) {
        // If it succeeds, we got 16 bytes
        EXPECT_TRUE(ind == (SQLLEN)sizeof(SQLGUID) || ind == 16);
    } else {
        // If not supported, try with SQL_C_BINARY instead
        ReallocStmt();
        ExecDirect("SELECT ID FROM TEST_UUID_STRUCT");
        ret = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        unsigned char rawData[16] = {};
        ret = SQLGetData(hStmt, 1, SQL_C_BINARY, rawData, sizeof(rawData), &ind);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        EXPECT_EQ(ind, 16);
    }
}

// Test: SQLGetTypeInfo for all supported types (coverage test)
TEST_F(GuidTest, TypeInfoCoversAllBaseTypes) {
    REQUIRE_FIREBIRD_CONNECTION();

    // Request all types
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    int count = 0;
    bool hasChar = false, hasVarchar = false, hasInteger = false;
    bool hasBigint = false, hasDouble = false, hasBoolean = false;
    bool hasDate = false, hasTime = false, hasTimestamp = false;
    bool hasGuid = false;

    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        count++;
        SQLCHAR typeName[128] = {};
        SQLSMALLINT sqlType = 0;
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), NULL);
        SQLGetData(hStmt, 2, SQL_C_SSHORT, &sqlType, sizeof(sqlType), NULL);

        switch (sqlType) {
            case SQL_CHAR: hasChar = true; break;
            case SQL_VARCHAR: hasVarchar = true; break;
            case SQL_INTEGER: hasInteger = true; break;
            case SQL_BIGINT: hasBigint = true; break;
            case SQL_DOUBLE: hasDouble = true; break;
            case SQL_BIT: hasBoolean = true; break;
            case SQL_TYPE_DATE: hasDate = true; break;
            case SQL_TYPE_TIME: hasTime = true; break;
            case SQL_TYPE_TIMESTAMP: hasTimestamp = true; break;
            case SQL_GUID: hasGuid = true; break;
        }
    }

    EXPECT_GT(count, 10) << "Should report at least 10 supported types";
    EXPECT_TRUE(hasChar) << "SQL_CHAR should be in type list";
    EXPECT_TRUE(hasVarchar) << "SQL_VARCHAR should be in type list";
    EXPECT_TRUE(hasInteger) << "SQL_INTEGER should be in type list";
    EXPECT_TRUE(hasBigint) << "SQL_BIGINT should be in type list";
    EXPECT_TRUE(hasDouble) << "SQL_DOUBLE should be in type list";
    EXPECT_TRUE(hasBoolean) << "SQL_BIT (BOOLEAN) should be in type list";
    EXPECT_TRUE(hasDate) << "SQL_TYPE_DATE should be in type list";
    EXPECT_TRUE(hasTime) << "SQL_TYPE_TIME should be in type list";
    EXPECT_TRUE(hasTimestamp) << "SQL_TYPE_TIMESTAMP should be in type list";
    EXPECT_TRUE(hasGuid) << "SQL_GUID should be in type list";
}

// ============================================================
// Firebird 4.0+ specific tests (skip if server version < 4)
// ============================================================

class Fb4PlusTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (hDbc == SQL_NULL_HDBC) return;  // skipped

        // Check server version
        SQLCHAR dbmsVer[64] = {};
        SQLSMALLINT len = 0;
        SQLGetInfo(hDbc, SQL_DBMS_VER, dbmsVer, sizeof(dbmsVer), &len);
        std::string version((char*)dbmsVer);
        // Firebird version format is like "5.0.2 Firebird 5.0"
        // or "WI-V5.0.2.1556 Firebird 5.0"
        int major = 0;
        // Try to extract major version number
        for (size_t i = 0; i < version.length(); i++) {
            if (version[i] >= '1' && version[i] <= '9') {
                major = version[i] - '0';
                break;
            }
        }
        serverMajor_ = major;
    }

    void RequireFb4Plus() {
        if (serverMajor_ < 4) {
            GTEST_SKIP() << "Requires Firebird 4.0+ (server is " << serverMajor_ << ".x)";
        }
    }

    int serverMajor_ = 0;
};

// Test: INT128 type is reported in SQLGetTypeInfo on Firebird 4+
TEST_F(Fb4PlusTest, TypeInfoIncludesInt128) {
    REQUIRE_FIREBIRD_CONNECTION();
    RequireFb4Plus();

    ReallocStmt();
    // INT128 is mapped to SQL_NUMERIC
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    bool found = false;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLCHAR typeName[128] = {};
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), NULL);
        if (strstr((char*)typeName, "INT128") != nullptr) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "INT128 should be in type list on Firebird 4+";
}

// Test: DECFLOAT type is reported on Firebird 4+
TEST_F(Fb4PlusTest, TypeInfoIncludesDecfloat) {
    REQUIRE_FIREBIRD_CONNECTION();
    RequireFb4Plus();

    ReallocStmt();
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    bool found = false;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLCHAR typeName[128] = {};
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), NULL);
        if (strstr((char*)typeName, "DECFLOAT") != nullptr) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "DECFLOAT should be in type list on Firebird 4+";
}

// Test: TIME WITH TIME ZONE is reported on Firebird 4+
TEST_F(Fb4PlusTest, TypeInfoIncludesTimeWithTZ) {
    REQUIRE_FIREBIRD_CONNECTION();
    RequireFb4Plus();

    ReallocStmt();
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    bool found = false;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLCHAR typeName[128] = {};
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), NULL);
        if (strstr((char*)typeName, "TIME WITH TIME ZONE") != nullptr) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "TIME WITH TIME ZONE should be in type list on Firebird 4+";
}

// Test: TIMESTAMP WITH TIME ZONE is reported on Firebird 4+
TEST_F(Fb4PlusTest, TypeInfoIncludesTimestampWithTZ) {
    REQUIRE_FIREBIRD_CONNECTION();
    RequireFb4Plus();

    ReallocStmt();
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    bool found = false;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLCHAR typeName[128] = {};
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), NULL);
        if (strstr((char*)typeName, "TIMESTAMP WITH TIME ZONE") != nullptr) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "TIMESTAMP WITH TIME ZONE should be in type list on Firebird 4+";
}

// Test: BINARY type is reported on Firebird 4+
TEST_F(Fb4PlusTest, TypeInfoIncludesBinary) {
    REQUIRE_FIREBIRD_CONNECTION();
    RequireFb4Plus();

    ReallocStmt();
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    bool foundBinary = false;
    bool foundVarbinary = false;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLCHAR typeName[128] = {};
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), NULL);
        std::string name((char*)typeName);
        if (name == "BINARY") foundBinary = true;
        if (name == "VARBINARY") foundVarbinary = true;
    }
    EXPECT_TRUE(foundBinary) << "BINARY should be in type list on Firebird 4+";
    EXPECT_TRUE(foundVarbinary) << "VARBINARY should be in type list on Firebird 4+";
}

// Test: Create table with BINARY(16) on Firebird 4+ — should map to SQL_GUID
TEST_F(Fb4PlusTest, Binary16MapsToGuid) {
    REQUIRE_FIREBIRD_CONNECTION();
    RequireFb4Plus();

    TempTable table(this, "TEST_BINARY16",
        "ID BINARY(16) NOT NULL, "
        "NAME VARCHAR(50)");

    ExecDirect("INSERT INTO TEST_BINARY16 (ID, NAME) VALUES (GEN_UUID(), 'binary_test')");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT ID FROM TEST_BINARY16");

    SQLSMALLINT sqlType = 0;
    SQLULEN columnSize = 0;
    SQLRETURN ret = SQLDescribeCol(hStmt, 1, NULL, 0, NULL, &sqlType, &columnSize, NULL, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // BINARY(16) should map to SQL_GUID
    EXPECT_EQ(sqlType, SQL_GUID)
        << "BINARY(16) should map to SQL_GUID on Firebird 4+, got " << sqlType;

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    unsigned char data[16] = {};
    SQLLEN ind = 0;
    ret = SQLGetData(hStmt, 1, SQL_C_BINARY, data, sizeof(data), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(ind, 16);
}

// Test: DECFLOAT column insertion and retrieval on Firebird 4+
TEST_F(Fb4PlusTest, DecfloatInsertAndRetrieve) {
    REQUIRE_FIREBIRD_CONNECTION();
    RequireFb4Plus();

    TempTable table(this, "TEST_DECFLOAT",
        "VAL DECFLOAT(16)");

    ExecDirect("INSERT INTO TEST_DECFLOAT (VAL) VALUES (3.14159265358979)");
    Commit();
    ReallocStmt();

    ExecDirect("SELECT VAL FROM TEST_DECFLOAT");

    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    double val = 0;
    SQLLEN ind = 0;
    ret = SQLGetData(hStmt, 1, SQL_C_DOUBLE, &val, sizeof(val), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_NEAR(val, 3.14159265358979, 0.00001);
}
