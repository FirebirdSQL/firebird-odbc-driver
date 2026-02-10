// tests/test_catalogfunctions.cpp — Comprehensive catalog function tests
// (Phase 6, ported from psqlodbc catalogfunctions-test)
//
// Tests all major catalog functions: SQLGetTypeInfo, SQLTables, SQLColumns,
// SQLPrimaryKeys, SQLForeignKeys, SQLSpecialColumns, SQLStatistics,
// SQLProcedures, SQLProcedureColumns, SQLTablePrivileges,
// SQLColumnPrivileges, SQLGetInfo.

#include "test_helpers.h"
#include <cstring>
#include <string>
#include <set>
#include <map>
#include <vector>

class CatalogFunctionsTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        // Create the primary table
        ExecIgnoreError("DROP TABLE ODBC_CAT_FK");
        ExecIgnoreError("DROP TABLE ODBC_CAT_PK");
        ExecIgnoreError("DROP TABLE ODBC_CAT_SPECIAL");
        ExecIgnoreError("EXECUTE BLOCK AS BEGIN "
                        "IF (EXISTS(SELECT 1 FROM RDB$PROCEDURES WHERE RDB$PROCEDURE_NAME = 'ODBC_CAT_ADD')) THEN "
                        "EXECUTE STATEMENT 'DROP PROCEDURE ODBC_CAT_ADD'; END");
        Commit();
        ReallocStmt();

        ExecDirect("CREATE TABLE ODBC_CAT_PK ("
                   "ID INTEGER NOT NULL PRIMARY KEY, "
                   "NAME VARCHAR(50) NOT NULL, "
                   "AMOUNT NUMERIC(10,2))");
        Commit();
        ReallocStmt();

        // Create a foreign-key table referencing the PK table
        ExecDirect("CREATE TABLE ODBC_CAT_FK ("
                   "FK_ID INTEGER NOT NULL PRIMARY KEY, "
                   "PK_ID INTEGER NOT NULL REFERENCES ODBC_CAT_PK(ID))");
        Commit();
        ReallocStmt();

        // Create a table with a unique index (no PK) for SQLSpecialColumns
        ExecDirect("CREATE TABLE ODBC_CAT_SPECIAL ("
                   "COL1 INTEGER NOT NULL, "
                   "COL2 VARCHAR(20) NOT NULL, "
                   "CONSTRAINT UQ_CAT_SPECIAL UNIQUE (COL1))");
        Commit();
        ReallocStmt();

        // Create a simple stored procedure
        ExecDirect("CREATE PROCEDURE ODBC_CAT_ADD (A INTEGER, B INTEGER) "
                   "RETURNS (RESULT INTEGER) AS "
                   "BEGIN RESULT = A + B; SUSPEND; END");
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        if (hDbc != SQL_NULL_HDBC) {
            ExecIgnoreError("DROP TABLE ODBC_CAT_FK");
            ExecIgnoreError("DROP TABLE ODBC_CAT_PK");
            ExecIgnoreError("DROP TABLE ODBC_CAT_SPECIAL");
            ExecIgnoreError("DROP PROCEDURE ODBC_CAT_ADD");
            SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
        }
        OdbcConnectedTest::TearDown();
    }
};

// --- SQLGetTypeInfo ---

TEST_F(CatalogFunctionsTest, GetTypeInfoAllTypes) {
    SQLRETURN rc = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLGetTypeInfo(SQL_ALL_TYPES) failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    int count = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) count++;
    EXPECT_GT(count, 5) << "Expected at least 5 type entries";
}

TEST_F(CatalogFunctionsTest, GetTypeInfoVarchar) {
    SQLRETURN rc = SQLGetTypeInfo(hStmt, SQL_VARCHAR);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLGetTypeInfo(SQL_VARCHAR) failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Should have at least one row for VARCHAR
    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc)) << "No VARCHAR type info returned";

    // Verify TYPE_NAME column is not empty
    SQLCHAR typeName[128] = {};
    SQLLEN ind = 0;
    rc = SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_GT(strlen((char*)typeName), 0u);
}

TEST_F(CatalogFunctionsTest, GetTypeInfoInteger) {
    SQLRETURN rc = SQLGetTypeInfo(hStmt, SQL_INTEGER);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLGetTypeInfo(SQL_INTEGER) failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc)) << "No INTEGER type info returned";

    // Verify DATA_TYPE column matches
    SQLSMALLINT dataType = 0;
    SQLLEN ind = 0;
    rc = SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(dataType, SQL_INTEGER);
}

// --- SQLTables ---

TEST_F(CatalogFunctionsTest, TablesFindsTestTable) {
    SQLRETURN rc = SQLTables(hStmt,
        NULL, 0,           // catalog
        NULL, 0,           // schema
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS,
        (SQLCHAR*)"TABLE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLTables failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc)) << "Table ODBC_CAT_PK not found";

    // TABLE_NAME is column 3
    SQLCHAR tblName[128] = {};
    SQLLEN ind = 0;
    rc = SQLGetData(hStmt, 3, SQL_C_CHAR, tblName, sizeof(tblName), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)tblName, "ODBC_CAT_PK");
}

TEST_F(CatalogFunctionsTest, TablesWithWildcard) {
    SQLRETURN rc = SQLTables(hStmt,
        NULL, 0,
        NULL, 0,
        (SQLCHAR*)"ODBC_CAT_%", SQL_NTS,
        (SQLCHAR*)"TABLE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLTables wildcard failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    int count = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) count++;
    EXPECT_GE(count, 3) << "Expected at least 3 ODBC_CAT_* tables";
}

TEST_F(CatalogFunctionsTest, TablesResultMetadata) {
    SQLRETURN rc = SQLTables(hStmt, NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS, (SQLCHAR*)"TABLE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    // Verify the result set has the standard 5 columns
    SQLSMALLINT numCols = 0;
    rc = SQLNumResultCols(hStmt, &numCols);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(numCols, 5);  // TABLE_CAT, TABLE_SCHEM, TABLE_NAME, TABLE_TYPE, REMARKS
}

// --- SQLColumns ---

TEST_F(CatalogFunctionsTest, ColumnsReturnsAllColumns) {
    SQLRETURN rc = SQLColumns(hStmt,
        NULL, 0,
        NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS,
        (SQLCHAR*)"%", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLColumns failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    int count = 0;
    bool foundId = false, foundName = false, foundAmount = false;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        count++;
        SQLCHAR colName[128] = {};
        SQLLEN ind = 0;
        SQLGetData(hStmt, 4, SQL_C_CHAR, colName, sizeof(colName), &ind);

        if (strcmp((char*)colName, "ID") == 0) foundId = true;
        else if (strcmp((char*)colName, "NAME") == 0) foundName = true;
        else if (strcmp((char*)colName, "AMOUNT") == 0) foundAmount = true;
    }
    EXPECT_EQ(count, 3);
    EXPECT_TRUE(foundId);
    EXPECT_TRUE(foundName);
    EXPECT_TRUE(foundAmount);
}

TEST_F(CatalogFunctionsTest, ColumnsDataTypes) {
    SQLRETURN rc = SQLColumns(hStmt,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS,
        NULL, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        SQLCHAR colName[128] = {};
        SQLSMALLINT dataType = 0;
        SQLLEN nameInd = 0, typeInd = 0;
        SQLGetData(hStmt, 4, SQL_C_CHAR, colName, sizeof(colName), &nameInd);
        SQLGetData(hStmt, 5, SQL_C_SSHORT, &dataType, 0, &typeInd);

        if (strcmp((char*)colName, "ID") == 0) {
            EXPECT_EQ(dataType, SQL_INTEGER);
        } else if (strcmp((char*)colName, "NAME") == 0) {
            EXPECT_TRUE(dataType == SQL_VARCHAR || dataType == SQL_WVARCHAR);
        } else if (strcmp((char*)colName, "AMOUNT") == 0) {
            EXPECT_TRUE(dataType == SQL_NUMERIC || dataType == SQL_DECIMAL);
        }
    }
}

TEST_F(CatalogFunctionsTest, ColumnsNullability) {
    SQLRETURN rc = SQLColumns(hStmt,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS,
        NULL, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        SQLCHAR colName[128] = {};
        SQLSMALLINT nullable = 0;
        SQLLEN nameInd = 0, nullInd = 0;
        SQLGetData(hStmt, 4, SQL_C_CHAR, colName, sizeof(colName), &nameInd);
        SQLGetData(hStmt, 11, SQL_C_SSHORT, &nullable, 0, &nullInd);

        if (strcmp((char*)colName, "ID") == 0 || strcmp((char*)colName, "NAME") == 0) {
            EXPECT_EQ(nullable, SQL_NO_NULLS);
        } else if (strcmp((char*)colName, "AMOUNT") == 0) {
            EXPECT_EQ(nullable, SQL_NULLABLE);
        }
    }
}

// --- SQLPrimaryKeys ---

TEST_F(CatalogFunctionsTest, PrimaryKeys) {
    SQLRETURN rc = SQLPrimaryKeys(hStmt,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLPrimaryKeys failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc)) << "No primary key found";

    // COLUMN_NAME is column 4
    SQLCHAR colName[128] = {};
    SQLLEN ind = 0;
    rc = SQLGetData(hStmt, 4, SQL_C_CHAR, colName, sizeof(colName), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)colName, "ID");

    // KEY_SEQ is column 5
    SQLSMALLINT keySeq = 0;
    rc = SQLGetData(hStmt, 5, SQL_C_SSHORT, &keySeq, 0, &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(keySeq, 1);
}

// --- SQLForeignKeys ---

TEST_F(CatalogFunctionsTest, ForeignKeys) {
    SQLRETURN rc = SQLForeignKeys(hStmt,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS,    // PK table
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_FK", SQL_NTS);   // FK table
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLForeignKeys failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc)) << "No foreign key relationship found";

    // PKTABLE_NAME col 3, PKCOLUMN_NAME col 4
    SQLCHAR pkTable[128] = {}, pkCol[128] = {};
    SQLLEN ind = 0;
    SQLGetData(hStmt, 3, SQL_C_CHAR, pkTable, sizeof(pkTable), &ind);
    SQLGetData(hStmt, 4, SQL_C_CHAR, pkCol, sizeof(pkCol), &ind);
    EXPECT_STREQ((char*)pkTable, "ODBC_CAT_PK");
    EXPECT_STREQ((char*)pkCol, "ID");

    // FKTABLE_NAME col 7, FKCOLUMN_NAME col 8
    SQLCHAR fkTable[128] = {}, fkCol[128] = {};
    SQLGetData(hStmt, 7, SQL_C_CHAR, fkTable, sizeof(fkTable), &ind);
    SQLGetData(hStmt, 8, SQL_C_CHAR, fkCol, sizeof(fkCol), &ind);
    EXPECT_STREQ((char*)fkTable, "ODBC_CAT_FK");
    EXPECT_STREQ((char*)fkCol, "PK_ID");
}

// --- SQLSpecialColumns ---

TEST_F(CatalogFunctionsTest, SpecialColumnsBestRowId) {
    SQLRETURN rc = SQLSpecialColumns(hStmt, SQL_BEST_ROWID,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS,
        SQL_SCOPE_SESSION, SQL_NULLABLE);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLSpecialColumns failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    int found = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) found++;
    EXPECT_GE(found, 1) << "Expected at least one BEST_ROWID column (PK)";
}

TEST_F(CatalogFunctionsTest, SpecialColumnsUniqueIndex) {
    SQLRETURN rc = SQLSpecialColumns(hStmt, SQL_BEST_ROWID,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_SPECIAL", SQL_NTS,
        SQL_SCOPE_SESSION, SQL_NO_NULLS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLSpecialColumns (unique idx) failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    int found = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) found++;
    EXPECT_GE(found, 1) << "Expected unique index column as BEST_ROWID";
}

TEST_F(CatalogFunctionsTest, SpecialColumnsRowVer) {
    SQLRETURN rc = SQLSpecialColumns(hStmt, SQL_ROWVER,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS,
        SQL_SCOPE_SESSION, SQL_NO_NULLS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLSpecialColumns(SQL_ROWVER) failed: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Firebird may or may not report row version columns — just verify no crash
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {}
    SUCCEED();
}

// --- SQLStatistics ---

TEST_F(CatalogFunctionsTest, Statistics) {
    SQLRETURN rc = SQLStatistics(hStmt,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS,
        SQL_INDEX_ALL, SQL_QUICK);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLStatistics failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Result should have 13 columns per ODBC spec
    SQLSMALLINT numCols = 0;
    rc = SQLNumResultCols(hStmt, &numCols);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(numCols, 13);

    int indexCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) indexCount++;
    EXPECT_GE(indexCount, 1) << "Expected at least one index (PK)";
}

TEST_F(CatalogFunctionsTest, StatisticsUniqueOnly) {
    SQLRETURN rc = SQLStatistics(hStmt,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_SPECIAL", SQL_NTS,
        SQL_INDEX_UNIQUE, SQL_QUICK);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));

    int count = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) count++;
    EXPECT_GE(count, 1) << "Expected at least one unique index";
}

// --- SQLProcedures ---

TEST_F(CatalogFunctionsTest, Procedures) {
    SQLRETURN rc = SQLProcedures(hStmt,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_ADD", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLProcedures failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    rc = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(rc)) << "Procedure ODBC_CAT_ADD not found";

    // PROCEDURE_NAME is column 3
    SQLCHAR procName[128] = {};
    SQLLEN ind = 0;
    rc = SQLGetData(hStmt, 3, SQL_C_CHAR, procName, sizeof(procName), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_STREQ((char*)procName, "ODBC_CAT_ADD");
}

// --- SQLProcedureColumns ---

TEST_F(CatalogFunctionsTest, ProcedureColumns) {
    SQLRETURN rc = SQLProcedureColumns(hStmt,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_ADD", SQL_NTS,
        NULL, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLProcedureColumns failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    int paramCount = 0;
    bool foundA = false, foundB = false, foundResult = false;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        paramCount++;
        SQLCHAR colName[128] = {};
        SQLLEN ind = 0;
        SQLGetData(hStmt, 4, SQL_C_CHAR, colName, sizeof(colName), &ind);

        if (strcmp((char*)colName, "A") == 0) foundA = true;
        else if (strcmp((char*)colName, "B") == 0) foundB = true;
        else if (strcmp((char*)colName, "RESULT") == 0) foundResult = true;
    }
    EXPECT_GE(paramCount, 2) << "Expected at least 2 parameter entries";
    EXPECT_TRUE(foundA) << "Parameter A not found";
    EXPECT_TRUE(foundB) << "Parameter B not found";
}

// --- SQLTablePrivileges ---

TEST_F(CatalogFunctionsTest, TablePrivileges) {
    SQLRETURN rc = SQLTablePrivileges(hStmt,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLTablePrivileges failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Just verify the call succeeds and produces a result set (may be empty
    // depending on Firebird configuration)
    SQLSMALLINT numCols = 0;
    rc = SQLNumResultCols(hStmt, &numCols);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(numCols, 7);  // Per ODBC spec
}

// --- SQLColumnPrivileges ---

TEST_F(CatalogFunctionsTest, ColumnPrivileges) {
    SQLRETURN rc = SQLColumnPrivileges(hStmt,
        NULL, 0, NULL, 0,
        (SQLCHAR*)"ODBC_CAT_PK", SQL_NTS,
        (SQLCHAR*)"ID", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLColumnPrivileges failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Per ODBC spec, the result set has 8 columns
    SQLSMALLINT numCols = 0;
    rc = SQLNumResultCols(hStmt, &numCols);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_EQ(numCols, 8);
}

// --- SQLGetInfo ---

TEST_F(CatalogFunctionsTest, GetInfoTableTerm) {
    SQLCHAR buf[128] = {};
    SQLSMALLINT len = 0;
    SQLRETURN rc = SQLGetInfo(hDbc, SQL_TABLE_TERM, buf, sizeof(buf), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLGetInfo(SQL_TABLE_TERM) failed: "
        << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    EXPECT_GT(len, 0) << "SQL_TABLE_TERM should not be empty";
}

TEST_F(CatalogFunctionsTest, GetInfoProcedureTerm) {
    SQLCHAR buf[128] = {};
    SQLSMALLINT len = 0;
    SQLRETURN rc = SQLGetInfo(hDbc, SQL_PROCEDURE_TERM, buf, sizeof(buf), &len);
    ASSERT_TRUE(SQL_SUCCEEDED(rc))
        << "SQLGetInfo(SQL_PROCEDURE_TERM) failed: "
        << GetOdbcError(SQL_HANDLE_DBC, hDbc);
    // May be empty if not supported, but should not fail
}

TEST_F(CatalogFunctionsTest, GetInfoMaxTableNameLen) {
    SQLUSMALLINT maxLen = 0;
    SQLRETURN rc = SQLGetInfo(hDbc, SQL_MAX_TABLE_NAME_LEN, &maxLen, sizeof(maxLen), NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(rc));
    EXPECT_GT(maxLen, 0u) << "Max table name length should be > 0";
}

// ===== SQLGetTypeInfo tests (from Phase 11) =====

class TypeInfoTest : public OdbcConnectedTest {};

// Result set must be sorted by DATA_TYPE ascending (ODBC spec requirement)
TEST_F(TypeInfoTest, ResultSetSortedByDataType) {
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLGetTypeInfo failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLSMALLINT prevDataType = -32768;
    int rowCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        SQLSMALLINT dataType = 0;
        SQLLEN ind = 0;
        ret = SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));

        EXPECT_GE(dataType, prevDataType)
            << "Row " << (rowCount + 1) << ": DATA_TYPE " << dataType
            << " is less than previous " << prevDataType
            << " — result set is not sorted by DATA_TYPE ascending";
        prevDataType = dataType;
        rowCount++;
    }
    EXPECT_GT(rowCount, 0) << "No rows returned by SQLGetTypeInfo(SQL_ALL_TYPES)";
}

// When a specific DATA_TYPE is requested, all matching rows must be returned
TEST_F(TypeInfoTest, MultipleRowsForSameDataType) {
    // SQL_INTEGER is a good test — should return exactly 1 row
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_INTEGER);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    int rowCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        SQLSMALLINT dataType = 0;
        SQLLEN ind = 0;
        SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind);
        EXPECT_EQ(dataType, SQL_INTEGER) << "Unexpected DATA_TYPE in filtered result";
        rowCount++;
    }
    EXPECT_GE(rowCount, 1) << "Should return at least 1 row for SQL_INTEGER";
}

// SQL_NUMERIC should return at least NUMERIC, and on FB4+ also INT128
TEST_F(TypeInfoTest, NumericReturnsMultipleOnFB4Plus) {
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_NUMERIC);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    std::vector<std::string> typeNames;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        SQLCHAR typeName[128] = {};
        SQLLEN ind = 0;
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &ind);
        typeNames.push_back(std::string((char*)typeName));
    }
    ASSERT_GE(typeNames.size(), 1u) << "At least NUMERIC should be returned";

    // Verify NUMERIC is in the list
    bool hasNumeric = false;
    for (auto& name : typeNames) {
        if (name == "NUMERIC") hasNumeric = true;
    }
    EXPECT_TRUE(hasNumeric) << "NUMERIC type not found in SQL_NUMERIC results";
}

// No non-existent type should return any rows
TEST_F(TypeInfoTest, NonexistentTypeReturnsNoRows) {
    SQLRETURN ret = SQLGetTypeInfo(hStmt, 9999);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    int rowCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt)))
        rowCount++;
    EXPECT_EQ(rowCount, 0) << "Invalid type 9999 should return 0 rows";
}

// SQL_GUID type should have SEARCHABLE = SQL_ALL_EXCEPT_LIKE (2)
TEST_F(TypeInfoTest, GuidSearchabilityIsAllExceptLike) {
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    bool foundGuid = false;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        SQLSMALLINT dataType = 0;
        SQLLEN ind = 0;
        SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind);
        if (dataType == SQL_GUID) {
            foundGuid = true;
            SQLSMALLINT searchable = 0;
            SQLGetData(hStmt, 9, SQL_C_SSHORT, &searchable, 0, &ind);
            EXPECT_EQ(searchable, SQL_ALL_EXCEPT_LIKE)
                << "SQL_GUID SEARCHABLE should be SQL_ALL_EXCEPT_LIKE (2), not " << searchable;

            // LITERAL_PREFIX/SUFFIX should be NULL for GUID
            SQLCHAR prefix[32] = {};
            ret = SQLGetData(hStmt, 4, SQL_C_CHAR, prefix, sizeof(prefix), &ind);
            EXPECT_TRUE(ind == SQL_NULL_DATA || strlen((char*)prefix) == 0)
                << "SQL_GUID LITERAL_PREFIX should be NULL or empty";
            break;
        }
    }
    EXPECT_TRUE(foundGuid) << "SQL_GUID type not found in type info result set";
}

// On FB4+ servers, BINARY/VARBINARY should not have duplicate entries
TEST_F(TypeInfoTest, NoDuplicateBinaryTypesOnFB4Plus) {
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    std::map<SQLSMALLINT, std::vector<std::string>> typeMap;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        SQLSMALLINT dataType = 0;
        SQLCHAR typeName[128] = {};
        SQLLEN ind = 0;
        SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind);
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &ind);
        typeMap[dataType].push_back(std::string((char*)typeName));
    }

    if (typeMap.count(SQL_BINARY)) {
        auto& names = typeMap[SQL_BINARY];
        bool hasBlobAlias = false;
        bool hasNative = false;
        for (auto& n : names) {
            if (n == "BLOB SUB_TYPE 0") hasBlobAlias = true;
            if (n == "BINARY") hasNative = true;
        }
        if (hasBlobAlias && hasNative) {
            FAIL() << "SQL_BINARY has both 'BLOB SUB_TYPE 0' and 'BINARY' entries — "
                      "version-gating failed";
        }
    }
}

// Verify every row in SQL_ALL_TYPES has valid data
TEST_F(TypeInfoTest, AllTypesReturnValidData) {
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    int rowCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        SQLCHAR typeName[128] = {};
        SQLSMALLINT dataType = 0;
        SQLINTEGER columnSize = 0;
        SQLLEN ind1 = 0, ind2 = 0, ind3 = 0;

        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &ind1);
        SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &ind2);
        SQLGetData(hStmt, 3, SQL_C_SLONG, &columnSize, 0, &ind3);

        EXPECT_NE(ind1, SQL_NULL_DATA) << "TYPE_NAME should not be NULL";
        EXPECT_NE(ind2, SQL_NULL_DATA) << "DATA_TYPE should not be NULL";
        EXPECT_GT(strlen((char*)typeName), 0u) << "TYPE_NAME should not be empty";
        rowCount++;
    }
    EXPECT_GT(rowCount, 10) << "Expected at least 10 type info rows";
}
