#include <gtest/gtest.h>

#ifdef _WIN32
#include <windows.h>
#include <string>
#endif
#include <sql.h>
#include <sqlext.h>

// =============================================================================
// Phase 0 Crash Prevention Tests
//
// These tests verify that the ODBC driver returns SQL_INVALID_HANDLE (or
// SQL_ERROR where appropriate) when called with NULL handles, instead of
// crashing via null pointer dereference.
//
// Issues addressed: C-1 (SQLCopyDesc crash), C-2 (GUARD_* dereference before
// null check), C-3 (no handle validation at entry points)
//
// IMPORTANT: These tests call the driver's exported functions DIRECTLY,
// bypassing the ODBC Driver Manager (DM). The DM itself may crash when
// given NULL handles because it needs a valid handle to determine which
// driver to dispatch to. Our Phase 0 fixes are in the driver's entry
// points (Main.cpp), so we must call them directly to test them.
// =============================================================================

#ifdef _WIN32

// ---------------------------------------------------------------------------
// Driver Direct-Call Infrastructure
//
// We load FirebirdODBC.dll directly and call its exported ODBC functions.
// This bypasses the Windows ODBC Driver Manager which may itself crash
// on NULL handles.
// ---------------------------------------------------------------------------

class NullHandleTests : public ::testing::Test {
protected:
    static HMODULE hDriver_;

    static void SetUpTestSuite() {
        // Determine the path to the driver DLL relative to the test executable
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        
        // Get the directory of the test executable
        std::string exeDir(exePath);
        auto lastSlash = exeDir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            exeDir = exeDir.substr(0, lastSlash);
        }
        
        // Try paths relative to the test executable directory
        const std::string paths[] = {
            exeDir + "\\FirebirdODBC.dll",
            exeDir + "\\..\\..\\Debug\\FirebirdODBC.dll",
            exeDir + "\\..\\..\\Release\\FirebirdODBC.dll",
            exeDir + "\\..\\Debug\\FirebirdODBC.dll",
            exeDir + "\\..\\Release\\FirebirdODBC.dll",
        };

        for (const auto& path : paths) {
            hDriver_ = LoadLibraryA(path.c_str());
            if (hDriver_) break;
        }
        ASSERT_NE(hDriver_, nullptr)
            << "Could not load FirebirdODBC.dll. "
            << "Ensure the driver is built and in the search path. "
            << "Last error: " << GetLastError();
    }

    static void TearDownTestSuite() {
        if (hDriver_) {
            FreeLibrary(hDriver_);
            hDriver_ = nullptr;
        }
    }

    // Helper to get a function pointer from the driver DLL
    template<typename FuncType>
    FuncType getDriverFunc(const char* name) {
        auto fn = reinterpret_cast<FuncType>(GetProcAddress(hDriver_, name));
        EXPECT_NE(fn, nullptr) << "Could not find " << name << " in driver DLL";
        return fn;
    }

    // Function pointer types for all ODBC functions we test
    using SQLBindCol_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN*);
    using SQLCancel_t = SQLRETURN (SQL_API*)(SQLHSTMT);
    using SQLColAttribute_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLUSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT*, SQLLEN*);
    using SQLDescribeCol_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*, SQLSMALLINT*, SQLULEN*, SQLSMALLINT*, SQLSMALLINT*);
    using SQLExecDirect_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLINTEGER);
    using SQLExecute_t = SQLRETURN (SQL_API*)(SQLHSTMT);
    using SQLFetch_t = SQLRETURN (SQL_API*)(SQLHSTMT);
    using SQLFetchScroll_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSMALLINT, SQLLEN);
    using SQLFreeStmt_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT);
    using SQLGetCursorName_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*);
    using SQLGetData_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN*);
    using SQLGetStmtAttr_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER*);
    using SQLGetTypeInfo_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSMALLINT);
    using SQLMoreResults_t = SQLRETURN (SQL_API*)(SQLHSTMT);
    using SQLNumResultCols_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSMALLINT*);
    using SQLPrepare_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLINTEGER);
    using SQLRowCount_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLLEN*);
    using SQLSetCursorName_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT);
    using SQLSetStmtAttr_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLINTEGER, SQLPOINTER, SQLINTEGER);
    using SQLCloseCursor_t = SQLRETURN (SQL_API*)(SQLHSTMT);
    using SQLColumns_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    using SQLTables_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    using SQLPrimaryKeys_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    using SQLForeignKeys_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    using SQLStatistics_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLUSMALLINT, SQLUSMALLINT);
    using SQLSpecialColumns_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLUSMALLINT, SQLUSMALLINT);
    using SQLBindParameter_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLULEN, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN*);
    using SQLNumParams_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSMALLINT*);
    using SQLDescribeParam_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT*, SQLULEN*, SQLSMALLINT*, SQLSMALLINT*);
    using SQLBulkOperations_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSMALLINT);
    using SQLSetPos_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSETPOSIROW, SQLUSMALLINT, SQLUSMALLINT);
    using SQLPutData_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLPOINTER, SQLLEN);
    using SQLParamData_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLPOINTER*);
    using SQLConnect_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    using SQLDriverConnect_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLHWND, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*, SQLUSMALLINT);
    using SQLDisconnect_t = SQLRETURN (SQL_API*)(SQLHDBC);
    using SQLGetConnectAttr_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER*);
    using SQLSetConnectAttr_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLINTEGER, SQLPOINTER, SQLINTEGER);
    using SQLGetInfo_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLUSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT*);
    using SQLGetFunctions_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLUSMALLINT, SQLUSMALLINT*);
    using SQLNativeSql_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLCHAR*, SQLINTEGER, SQLCHAR*, SQLINTEGER, SQLINTEGER*);
    using SQLEndTran_t = SQLRETURN (SQL_API*)(SQLSMALLINT, SQLHANDLE, SQLSMALLINT);
    using SQLBrowseConnect_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*);
    using SQLGetEnvAttr_t = SQLRETURN (SQL_API*)(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER*);
    using SQLSetEnvAttr_t = SQLRETURN (SQL_API*)(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER);
    using SQLCopyDesc_t = SQLRETURN (SQL_API*)(SQLHDESC, SQLHDESC);
    using SQLGetDescField_t = SQLRETURN (SQL_API*)(SQLHDESC, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLINTEGER, SQLINTEGER*);
    using SQLGetDescRec_t = SQLRETURN (SQL_API*)(SQLHDESC, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*, SQLSMALLINT*, SQLSMALLINT*, SQLLEN*, SQLSMALLINT*, SQLSMALLINT*, SQLSMALLINT*);
    using SQLSetDescField_t = SQLRETURN (SQL_API*)(SQLHDESC, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLINTEGER);
    using SQLSetDescRec_t = SQLRETURN (SQL_API*)(SQLHDESC, SQLSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLLEN, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN*, SQLLEN*);
    using SQLFreeHandle_t = SQLRETURN (SQL_API*)(SQLSMALLINT, SQLHANDLE);
    using SQLAllocHandle_t = SQLRETURN (SQL_API*)(SQLSMALLINT, SQLHANDLE, SQLHANDLE*);
    using SQLGetDiagRec_t = SQLRETURN (SQL_API*)(SQLSMALLINT, SQLHANDLE, SQLSMALLINT, SQLCHAR*, SQLINTEGER*, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*);
    using SQLGetDiagField_t = SQLRETURN (SQL_API*)(SQLSMALLINT, SQLHANDLE, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT*);
    using SQLAllocConnect_t = SQLRETURN (SQL_API*)(SQLHENV, SQLHDBC*);
    using SQLAllocStmt_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLHSTMT*);
    using SQLFreeConnect_t = SQLRETURN (SQL_API*)(SQLHDBC);
    using SQLFreeEnv_t = SQLRETURN (SQL_API*)(SQLHENV);
};

HMODULE NullHandleTests::hDriver_ = nullptr;

#else
// =============================================================================
// Linux/macOS: Direct-call via dlopen/dlsym
// =============================================================================

#include <dlfcn.h>

class NullHandleTests : public ::testing::Test {
protected:
    static void* hDriver_;

    static void SetUpTestSuite() {
        const char* paths[] = {
            "./libOdbcFb.so",
            "../libOdbcFb.so",
            "../../libOdbcFb.so",
            "./OdbcFb.so",
            "../OdbcFb.so",
        };

        for (auto path : paths) {
            hDriver_ = dlopen(path, RTLD_NOW | RTLD_NODELETE);
            if (hDriver_) break;
        }
        ASSERT_NE(hDriver_, nullptr)
            << "Could not load libOdbcFb.so. "
            << "Ensure the driver is built and in the search path. "
            << "dlerror: " << dlerror();
    }

    static void TearDownTestSuite() {
        // Don't dlclose the driver - it can cause "double free" crashes
        // during process teardown when the driver's global destructors
        // conflict with the test process cleanup. The OS will clean up
        // when the process exits.
        hDriver_ = nullptr;
    }

    template<typename FuncType>
    FuncType getDriverFunc(const char* name) {
        auto fn = reinterpret_cast<FuncType>(dlsym(hDriver_, name));
        EXPECT_NE(fn, nullptr) << "Could not find " << name << " in driver .so: " << dlerror();
        return fn;
    }

    // Same function pointer types as Windows
    using SQLBindCol_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN*);
    using SQLCancel_t = SQLRETURN (SQL_API*)(SQLHSTMT);
    using SQLColAttribute_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLUSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT*, SQLLEN*);
    using SQLDescribeCol_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*, SQLSMALLINT*, SQLULEN*, SQLSMALLINT*, SQLSMALLINT*);
    using SQLExecDirect_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLINTEGER);
    using SQLExecute_t = SQLRETURN (SQL_API*)(SQLHSTMT);
    using SQLFetch_t = SQLRETURN (SQL_API*)(SQLHSTMT);
    using SQLFetchScroll_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSMALLINT, SQLLEN);
    using SQLFreeStmt_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT);
    using SQLGetCursorName_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*);
    using SQLGetData_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN*);
    using SQLGetStmtAttr_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER*);
    using SQLGetTypeInfo_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSMALLINT);
    using SQLMoreResults_t = SQLRETURN (SQL_API*)(SQLHSTMT);
    using SQLNumResultCols_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSMALLINT*);
    using SQLPrepare_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLINTEGER);
    using SQLRowCount_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLLEN*);
    using SQLSetCursorName_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT);
    using SQLSetStmtAttr_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLINTEGER, SQLPOINTER, SQLINTEGER);
    using SQLCloseCursor_t = SQLRETURN (SQL_API*)(SQLHSTMT);
    using SQLColumns_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    using SQLTables_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    using SQLPrimaryKeys_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    using SQLForeignKeys_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    using SQLStatistics_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLUSMALLINT, SQLUSMALLINT);
    using SQLSpecialColumns_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLUSMALLINT, SQLUSMALLINT);
    using SQLBindParameter_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLULEN, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN*);
    using SQLNumParams_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSMALLINT*);
    using SQLDescribeParam_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT*, SQLULEN*, SQLSMALLINT*, SQLSMALLINT*);
    using SQLBulkOperations_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSMALLINT);
    using SQLSetPos_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLSETPOSIROW, SQLUSMALLINT, SQLUSMALLINT);
    using SQLPutData_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLPOINTER, SQLLEN);
    using SQLParamData_t = SQLRETURN (SQL_API*)(SQLHSTMT, SQLPOINTER*);
    using SQLConnect_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    using SQLDriverConnect_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLHWND, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*, SQLUSMALLINT);
    using SQLDisconnect_t = SQLRETURN (SQL_API*)(SQLHDBC);
    using SQLGetConnectAttr_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER*);
    using SQLSetConnectAttr_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLINTEGER, SQLPOINTER, SQLINTEGER);
    using SQLGetInfo_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLUSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT*);
    using SQLGetFunctions_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLUSMALLINT, SQLUSMALLINT*);
    using SQLNativeSql_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLCHAR*, SQLINTEGER, SQLCHAR*, SQLINTEGER, SQLINTEGER*);
    using SQLEndTran_t = SQLRETURN (SQL_API*)(SQLSMALLINT, SQLHANDLE, SQLSMALLINT);
    using SQLBrowseConnect_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*);
    using SQLGetEnvAttr_t = SQLRETURN (SQL_API*)(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER*);
    using SQLSetEnvAttr_t = SQLRETURN (SQL_API*)(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER);
    using SQLCopyDesc_t = SQLRETURN (SQL_API*)(SQLHDESC, SQLHDESC);
    using SQLGetDescField_t = SQLRETURN (SQL_API*)(SQLHDESC, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLINTEGER, SQLINTEGER*);
    using SQLGetDescRec_t = SQLRETURN (SQL_API*)(SQLHDESC, SQLSMALLINT, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*, SQLSMALLINT*, SQLSMALLINT*, SQLLEN*, SQLSMALLINT*, SQLSMALLINT*, SQLSMALLINT*);
    using SQLSetDescField_t = SQLRETURN (SQL_API*)(SQLHDESC, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLINTEGER);
    using SQLSetDescRec_t = SQLRETURN (SQL_API*)(SQLHDESC, SQLSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLLEN, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN*, SQLLEN*);
    using SQLFreeHandle_t = SQLRETURN (SQL_API*)(SQLSMALLINT, SQLHANDLE);
    using SQLAllocHandle_t = SQLRETURN (SQL_API*)(SQLSMALLINT, SQLHANDLE, SQLHANDLE*);
    using SQLGetDiagRec_t = SQLRETURN (SQL_API*)(SQLSMALLINT, SQLHANDLE, SQLSMALLINT, SQLCHAR*, SQLINTEGER*, SQLCHAR*, SQLSMALLINT, SQLSMALLINT*);
    using SQLGetDiagField_t = SQLRETURN (SQL_API*)(SQLSMALLINT, SQLHANDLE, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT*);
    using SQLAllocConnect_t = SQLRETURN (SQL_API*)(SQLHENV, SQLHDBC*);
    using SQLAllocStmt_t = SQLRETURN (SQL_API*)(SQLHDBC, SQLHSTMT*);
    using SQLFreeConnect_t = SQLRETURN (SQL_API*)(SQLHDBC);
    using SQLFreeEnv_t = SQLRETURN (SQL_API*)(SQLHENV);
};

void* NullHandleTests::hDriver_ = nullptr;

#endif // _WIN32

// ---------------------------------------------------------------------------
// Statement handle (HSTMT) entry points with NULL
// ---------------------------------------------------------------------------

TEST_F(NullHandleTests, SQLBindColNullStmt)
{
    auto fn = getDriverFunc<SQLBindCol_t>("SQLBindCol");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, 1, SQL_C_CHAR, nullptr, 0, nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLCancelNullStmt)
{
    auto fn = getDriverFunc<SQLCancel_t>("SQLCancel");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLColAttributeNullStmt)
{
    auto fn = getDriverFunc<SQLColAttribute_t>("SQLColAttribute");
    if (!fn) return;
    SQLSMALLINT stringLength = 0;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, 1, SQL_DESC_NAME,
                      nullptr, 0, &stringLength, nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLDescribeColNullStmt)
{
    auto fn = getDriverFunc<SQLDescribeCol_t>("SQLDescribeCol");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, 1, nullptr, 0, nullptr,
                      nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLExecDirectNullStmt)
{
    auto fn = getDriverFunc<SQLExecDirect_t>("SQLExecDirect");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, (SQLCHAR*)"SELECT 1", SQL_NTS);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLExecuteNullStmt)
{
    auto fn = getDriverFunc<SQLExecute_t>("SQLExecute");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLFetchNullStmt)
{
    auto fn = getDriverFunc<SQLFetch_t>("SQLFetch");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLFetchScrollNullStmt)
{
    auto fn = getDriverFunc<SQLFetchScroll_t>("SQLFetchScroll");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, SQL_FETCH_NEXT, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLFreeStmtNullStmt)
{
    auto fn = getDriverFunc<SQLFreeStmt_t>("SQLFreeStmt");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, SQL_CLOSE);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLGetCursorNameNullStmt)
{
    auto fn = getDriverFunc<SQLGetCursorName_t>("SQLGetCursorName");
    if (!fn) return;
    SQLCHAR name[128];
    SQLSMALLINT nameLen;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, name, sizeof(name), &nameLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLGetDataNullStmt)
{
    auto fn = getDriverFunc<SQLGetData_t>("SQLGetData");
    if (!fn) return;
    char buf[32];
    SQLLEN ind;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLGetStmtAttrNullStmt)
{
    auto fn = getDriverFunc<SQLGetStmtAttr_t>("SQLGetStmtAttr");
    if (!fn) return;
    SQLINTEGER value;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, SQL_ATTR_ROW_NUMBER,
                      &value, sizeof(value), nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLGetTypeInfoNullStmt)
{
    auto fn = getDriverFunc<SQLGetTypeInfo_t>("SQLGetTypeInfo");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, SQL_ALL_TYPES);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLMoreResultsNullStmt)
{
    auto fn = getDriverFunc<SQLMoreResults_t>("SQLMoreResults");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLNumResultColsNullStmt)
{
    auto fn = getDriverFunc<SQLNumResultCols_t>("SQLNumResultCols");
    if (!fn) return;
    SQLSMALLINT cols;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, &cols);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLPrepareNullStmt)
{
    auto fn = getDriverFunc<SQLPrepare_t>("SQLPrepare");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, (SQLCHAR*)"SELECT 1", SQL_NTS);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLRowCountNullStmt)
{
    auto fn = getDriverFunc<SQLRowCount_t>("SQLRowCount");
    if (!fn) return;
    SQLLEN count;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, &count);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLSetCursorNameNullStmt)
{
    auto fn = getDriverFunc<SQLSetCursorName_t>("SQLSetCursorName");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, (SQLCHAR*)"test", SQL_NTS);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLSetStmtAttrNullStmt)
{
    auto fn = getDriverFunc<SQLSetStmtAttr_t>("SQLSetStmtAttr");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, SQL_ATTR_QUERY_TIMEOUT,
                      (SQLPOINTER)10, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLCloseCursorNullStmt)
{
    auto fn = getDriverFunc<SQLCloseCursor_t>("SQLCloseCursor");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLColumnsNullStmt)
{
    auto fn = getDriverFunc<SQLColumns_t>("SQLColumns");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, nullptr, 0, nullptr, 0,
                      nullptr, 0, nullptr, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLTablesNullStmt)
{
    auto fn = getDriverFunc<SQLTables_t>("SQLTables");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, nullptr, 0, nullptr, 0,
                      nullptr, 0, nullptr, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLPrimaryKeysNullStmt)
{
    auto fn = getDriverFunc<SQLPrimaryKeys_t>("SQLPrimaryKeys");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, nullptr, 0, nullptr, 0,
                      nullptr, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLForeignKeysNullStmt)
{
    auto fn = getDriverFunc<SQLForeignKeys_t>("SQLForeignKeys");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, nullptr, 0, nullptr, 0,
                      nullptr, 0, nullptr, 0, nullptr, 0,
                      nullptr, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLStatisticsNullStmt)
{
    auto fn = getDriverFunc<SQLStatistics_t>("SQLStatistics");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, nullptr, 0, nullptr, 0,
                      nullptr, 0, SQL_INDEX_ALL, SQL_QUICK);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLSpecialColumnsNullStmt)
{
    auto fn = getDriverFunc<SQLSpecialColumns_t>("SQLSpecialColumns");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, SQL_BEST_ROWID,
                      nullptr, 0, nullptr, 0, nullptr, 0,
                      SQL_SCOPE_SESSION, SQL_NULLABLE);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLBindParameterNullStmt)
{
    auto fn = getDriverFunc<SQLBindParameter_t>("SQLBindParameter");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, 1, SQL_PARAM_INPUT,
                      SQL_C_LONG, SQL_INTEGER, 0, 0,
                      nullptr, 0, nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLNumParamsNullStmt)
{
    auto fn = getDriverFunc<SQLNumParams_t>("SQLNumParams");
    if (!fn) return;
    SQLSMALLINT params;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, &params);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLDescribeParamNullStmt)
{
    auto fn = getDriverFunc<SQLDescribeParam_t>("SQLDescribeParam");
    if (!fn) return;
    SQLSMALLINT type;
    SQLULEN size;
    SQLSMALLINT digits, nullable;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, 1, &type, &size,
                      &digits, &nullable);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLBulkOperationsNullStmt)
{
    auto fn = getDriverFunc<SQLBulkOperations_t>("SQLBulkOperations");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, SQL_ADD);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLSetPosNullStmt)
{
    auto fn = getDriverFunc<SQLSetPos_t>("SQLSetPos");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLPutDataNullStmt)
{
    auto fn = getDriverFunc<SQLPutData_t>("SQLPutData");
    if (!fn) return;
    int data = 42;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, &data, sizeof(data));
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLParamDataNullStmt)
{
    auto fn = getDriverFunc<SQLParamData_t>("SQLParamData");
    if (!fn) return;
    SQLPOINTER value;
    SQLRETURN rc = fn(SQL_NULL_HSTMT, &value);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// Connection handle (HDBC) entry points with NULL
// ---------------------------------------------------------------------------

TEST_F(NullHandleTests, SQLConnectNullDbc)
{
    auto fn = getDriverFunc<SQLConnect_t>("SQLConnect");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HDBC, (SQLCHAR*)"test", SQL_NTS,
                      (SQLCHAR*)"user", SQL_NTS,
                      (SQLCHAR*)"pass", SQL_NTS);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLDriverConnectNullDbc)
{
    auto fn = getDriverFunc<SQLDriverConnect_t>("SQLDriverConnect");
    if (!fn) return;
    SQLCHAR outStr[256];
    SQLSMALLINT outLen;
    SQLRETURN rc = fn(SQL_NULL_HDBC, nullptr,
                      (SQLCHAR*)"DSN=test", SQL_NTS,
                      outStr, sizeof(outStr), &outLen,
                      SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLDisconnectNullDbc)
{
    auto fn = getDriverFunc<SQLDisconnect_t>("SQLDisconnect");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HDBC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLGetConnectAttrNullDbc)
{
    auto fn = getDriverFunc<SQLGetConnectAttr_t>("SQLGetConnectAttr");
    if (!fn) return;
    SQLINTEGER value;
    SQLRETURN rc = fn(SQL_NULL_HDBC, SQL_ATTR_AUTOCOMMIT,
                      &value, sizeof(value), nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLSetConnectAttrNullDbc)
{
    auto fn = getDriverFunc<SQLSetConnectAttr_t>("SQLSetConnectAttr");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HDBC, SQL_ATTR_AUTOCOMMIT,
                      (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLGetInfoNullDbc)
{
    auto fn = getDriverFunc<SQLGetInfo_t>("SQLGetInfo");
    if (!fn) return;
    char buf[128];
    SQLSMALLINT len;
    SQLRETURN rc = fn(SQL_NULL_HDBC, SQL_DBMS_NAME, buf,
                      sizeof(buf), &len);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLGetFunctionsNullDbc)
{
    auto fn = getDriverFunc<SQLGetFunctions_t>("SQLGetFunctions");
    if (!fn) return;
    SQLUSMALLINT supported;
    SQLRETURN rc = fn(SQL_NULL_HDBC, SQL_API_SQLBINDCOL, &supported);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLNativeSqlNullDbc)
{
    auto fn = getDriverFunc<SQLNativeSql_t>("SQLNativeSql");
    if (!fn) return;
    SQLCHAR out[128];
    SQLINTEGER outLen;
    SQLRETURN rc = fn(SQL_NULL_HDBC, (SQLCHAR*)"SELECT 1", SQL_NTS,
                      out, sizeof(out), &outLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLEndTranNullDbc)
{
    auto fn = getDriverFunc<SQLEndTran_t>("SQLEndTran");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_HANDLE_DBC, SQL_NULL_HDBC, SQL_COMMIT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLBrowseConnectNullDbc)
{
    auto fn = getDriverFunc<SQLBrowseConnect_t>("SQLBrowseConnect");
    if (!fn) return;
    SQLCHAR outStr[256];
    SQLSMALLINT outLen;
    SQLRETURN rc = fn(SQL_NULL_HDBC, (SQLCHAR*)"DSN=test", SQL_NTS,
                      outStr, sizeof(outStr), &outLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// Environment handle (HENV) entry points with NULL
// ---------------------------------------------------------------------------

TEST_F(NullHandleTests, SQLGetEnvAttrNullEnv)
{
    auto fn = getDriverFunc<SQLGetEnvAttr_t>("SQLGetEnvAttr");
    if (!fn) return;
    SQLINTEGER value;
    SQLRETURN rc = fn(SQL_NULL_HENV, SQL_ATTR_ODBC_VERSION,
                      &value, sizeof(value), nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLSetEnvAttrNullEnv)
{
    auto fn = getDriverFunc<SQLSetEnvAttr_t>("SQLSetEnvAttr");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HENV, SQL_ATTR_ODBC_VERSION,
                      (SQLPOINTER)SQL_OV_ODBC3, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLEndTranNullEnv)
{
    auto fn = getDriverFunc<SQLEndTran_t>("SQLEndTran");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_HANDLE_ENV, SQL_NULL_HENV, SQL_COMMIT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// Descriptor handle (HDESC) entry points with NULL — Issue C-1
// ---------------------------------------------------------------------------

TEST_F(NullHandleTests, SQLCopyDescNullSource)
{
    auto fn = getDriverFunc<SQLCopyDesc_t>("SQLCopyDesc");
    if (!fn) return;
    // C-1: This previously crashed with access violation
    SQLRETURN rc = fn(SQL_NULL_HDESC, SQL_NULL_HDESC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLCopyDescNullTarget)
{
    auto fn = getDriverFunc<SQLCopyDesc_t>("SQLCopyDesc");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HDESC, SQL_NULL_HDESC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLGetDescFieldNullDesc)
{
    auto fn = getDriverFunc<SQLGetDescField_t>("SQLGetDescField");
    if (!fn) return;
    SQLINTEGER value;
    SQLINTEGER strLen;
    SQLRETURN rc = fn(SQL_NULL_HDESC, 1, SQL_DESC_COUNT,
                      &value, sizeof(value), &strLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLGetDescRecNullDesc)
{
    auto fn = getDriverFunc<SQLGetDescRec_t>("SQLGetDescRec");
    if (!fn) return;
    SQLCHAR name[128];
    SQLSMALLINT nameLen, type, subType, precision, scale, nullable;
    SQLLEN length;
    SQLRETURN rc = fn(SQL_NULL_HDESC, 1, name, sizeof(name),
                      &nameLen, &type, &subType, &length,
                      &precision, &scale, &nullable);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLSetDescFieldNullDesc)
{
    auto fn = getDriverFunc<SQLSetDescField_t>("SQLSetDescField");
    if (!fn) return;
    SQLINTEGER value = 0;
    SQLRETURN rc = fn(SQL_NULL_HDESC, 1, SQL_DESC_TYPE,
                      &value, sizeof(value));
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLSetDescRecNullDesc)
{
    auto fn = getDriverFunc<SQLSetDescRec_t>("SQLSetDescRec");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HDESC, 1, SQL_INTEGER, 0,
                      4, 0, 0, nullptr, nullptr, nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// SQLFreeHandle with NULL handles
// ---------------------------------------------------------------------------

TEST_F(NullHandleTests, SQLFreeHandleNullEnv)
{
    auto fn = getDriverFunc<SQLFreeHandle_t>("SQLFreeHandle");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_HANDLE_ENV, SQL_NULL_HENV);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLFreeHandleNullDbc)
{
    auto fn = getDriverFunc<SQLFreeHandle_t>("SQLFreeHandle");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_HANDLE_DBC, SQL_NULL_HDBC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLFreeHandleNullStmt)
{
    auto fn = getDriverFunc<SQLFreeHandle_t>("SQLFreeHandle");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_HANDLE_STMT, SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLFreeHandleNullDesc)
{
    auto fn = getDriverFunc<SQLFreeHandle_t>("SQLFreeHandle");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_HANDLE_DESC, SQL_NULL_HDESC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLFreeHandleInvalidType)
{
    auto fn = getDriverFunc<SQLFreeHandle_t>("SQLFreeHandle");
    if (!fn) return;
    SQLRETURN rc = fn(999, SQL_NULL_HANDLE);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// SQLAllocHandle with NULL input handles
// ---------------------------------------------------------------------------

TEST_F(NullHandleTests, SQLAllocHandleDbc_NullEnv)
{
    auto fn = getDriverFunc<SQLAllocHandle_t>("SQLAllocHandle");
    if (!fn) return;
    SQLHANDLE output;
    SQLRETURN rc = fn(SQL_HANDLE_DBC, SQL_NULL_HENV, &output);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLAllocHandleStmt_NullDbc)
{
    auto fn = getDriverFunc<SQLAllocHandle_t>("SQLAllocHandle");
    if (!fn) return;
    SQLHANDLE output;
    SQLRETURN rc = fn(SQL_HANDLE_STMT, SQL_NULL_HDBC, &output);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// SQLGetDiagRec / SQLGetDiagField with NULL handles
// ---------------------------------------------------------------------------

TEST_F(NullHandleTests, SQLGetDiagRecNullHandle)
{
    auto fn = getDriverFunc<SQLGetDiagRec_t>("SQLGetDiagRec");
    if (!fn) return;
    SQLCHAR sqlState[6];
    SQLINTEGER nativeError;
    SQLCHAR message[256];
    SQLSMALLINT msgLen;
    SQLRETURN rc = fn(SQL_HANDLE_STMT, SQL_NULL_HSTMT, 1,
                      sqlState, &nativeError, message,
                      sizeof(message), &msgLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLGetDiagFieldNullHandle)
{
    auto fn = getDriverFunc<SQLGetDiagField_t>("SQLGetDiagField");
    if (!fn) return;
    SQLINTEGER value;
    SQLSMALLINT strLen;
    SQLRETURN rc = fn(SQL_HANDLE_STMT, SQL_NULL_HSTMT, 0,
                      SQL_DIAG_NUMBER, &value, sizeof(value),
                      &strLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// Deprecated ODBC 1.0 functions with NULL handles
// ---------------------------------------------------------------------------

TEST_F(NullHandleTests, SQLAllocConnectNullEnv)
{
    auto fn = getDriverFunc<SQLAllocConnect_t>("SQLAllocConnect");
    if (!fn) return;
    SQLHDBC hDbc;
    SQLRETURN rc = fn(SQL_NULL_HENV, &hDbc);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLAllocStmtNullDbc)
{
    auto fn = getDriverFunc<SQLAllocStmt_t>("SQLAllocStmt");
    if (!fn) return;
    SQLHSTMT hStmt;
    SQLRETURN rc = fn(SQL_NULL_HDBC, &hStmt);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLFreeConnectNullDbc)
{
    auto fn = getDriverFunc<SQLFreeConnect_t>("SQLFreeConnect");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HDBC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST_F(NullHandleTests, SQLFreeEnvNullEnv)
{
    auto fn = getDriverFunc<SQLFreeEnv_t>("SQLFreeEnv");
    if (!fn) return;
    SQLRETURN rc = fn(SQL_NULL_HENV);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}
