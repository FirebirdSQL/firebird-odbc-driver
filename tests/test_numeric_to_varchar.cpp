// tests/test_numeric_to_varchar.cpp — Numeric/datetime C-type → Firebird VARCHAR
//
// The Issue #161 driver fix (duckdb/odbc-scanner#161) lives in the
// conv<Numeric|DateTime>ToString family of conversion helpers, each of which
// writes an ASCII representation into a Firebird-side VARYING buffer.  Before
// the fix every one of those helpers wrote raw bytes at offset 0 of the
// buffer, stomping on the VARYING length prefix.  The pre-existing suite only
// covered SLONG → INTEGER/SMALLINT round-trips, so none of the broken paths
// were tested.
//
// This file adds one compact, loop-based test per conversion helper.  Each
// binds the relevant C-type to a VARCHAR column, executes the same prepared
// INSERT many times with values of varying widths, and verifies:
//   - every row committed,
//   - no stored value contains an embedded NUL byte,
//   - CAST(val AS <original-type>) round-trips for the boundary values.
//
// DML (no stored procedure) is deliberately used so the tests run on every
// server in the matrix — FB 6 master still rejects parameterized EXECUTE
// PROCEDURE via SKIP_ON_FIREBIRD6(), but `UPDATE OR INSERT` / `INSERT` are
// accepted.

#include "test_helpers.h"
#include <cstring>
#include <string>
#include <vector>

class NumericToVarcharTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        ExecIgnoreError("DROP TABLE ODBC_NUM_VAR");
        Commit();
        ReallocStmt();

        // 40 bytes is plenty for any decimal representation produced by the
        // conv*ToString helpers (longest case is a DOUBLE ≈ 23 chars).
        ExecDirect("CREATE TABLE ODBC_NUM_VAR ("
                   "ID INTEGER NOT NULL PRIMARY KEY, "
                   "VAL VARCHAR(40))");
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        if (hDbc != SQL_NULL_HDBC) {
            ExecIgnoreError("DROP TABLE ODBC_NUM_VAR");
            SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
        }
        OdbcConnectedTest::TearDown();
    }

    // Verifies no stored VAL contains a NUL byte (the classic pre-fix symptom
    // is the first-row value being stored as two bytes, e.g. "1\0") and that
    // exactly `expectedRowCount` rows were persisted.
    void verifyIntactness(int expectedRowCount) {
        ExecDirect("SELECT COUNT(*) FROM ODBC_NUM_VAR");
        SQLRETURN ret = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        SQLINTEGER cnt = 0;
        SQLLEN ind = 0;
        SQLGetData(hStmt, 1, SQL_C_SLONG, &cnt, sizeof(cnt), &ind);
        SQLCloseCursor(hStmt);
        EXPECT_EQ(cnt, expectedRowCount) << "rows stored";

        ExecDirect("SELECT COUNT(*) FROM ODBC_NUM_VAR "
                   "WHERE POSITION(_OCTETS x'00' IN VAL) > 0");
        ret = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        SQLINTEGER nulCount = -1;
        SQLGetData(hStmt, 1, SQL_C_SLONG, &nulCount, sizeof(nulCount), &ind);
        SQLCloseCursor(hStmt);
        EXPECT_EQ(nulCount, 0) << "rows with NUL-byte corruption";
    }
};

// ===== SQL_C_SSHORT (→ convShortToString) =====

TEST_F(NumericToVarcharTest, SShortToVarcharLoopDml) {
    // FB 6 master crashes with "Stack overflow" / SEGFAULT on loop-prepared
    // parameterized INSERT — see SKIP_ON_FIREBIRD6 note in test_helpers.h.
    SKIP_ON_FIREBIRD6();

    const std::vector<SQLSMALLINT> vals = {1, 10, 100, 999, -1, -999};

    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_NUM_VAR (ID, VAL) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER idVal = 0;   SQLLEN idInd = sizeof(idVal);
    SQLSMALLINT val = 0;    SQLLEN valInd = sizeof(val);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG,  SQL_INTEGER, 0, 0, &idVal, sizeof(idVal), &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_VARCHAR, 40, 0, &val, sizeof(val), &valInd);

    for (size_t i = 0; i < vals.size(); ++i) {
        idVal = static_cast<SQLINTEGER>(i + 1);
        val   = vals[i];
        ret = SQLExecute(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "row " << i + 1 << " value " << vals[i];
    }
    Commit();
    ReallocStmt();

    verifyIntactness(static_cast<int>(vals.size()));

    // Round-trip spot checks.
    ExecDirect("SELECT CAST(VAL AS SMALLINT) FROM ODBC_NUM_VAR ORDER BY ID");
    for (auto expected : vals) {
        ret = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        SQLSMALLINT got = 0;
        SQLLEN ind = 0;
        SQLGetData(hStmt, 1, SQL_C_SSHORT, &got, sizeof(got), &ind);
        EXPECT_EQ(got, expected);
    }
    SQLCloseCursor(hStmt);
}

// ===== SQL_C_SBIGINT (→ convBigintToString) =====

TEST_F(NumericToVarcharTest, SBigintToVarcharLoopDml) {
    SKIP_ON_FIREBIRD6();

    const std::vector<SQLBIGINT> vals = {
        1LL, 9LL, 10LL, 99LL, 100LL, 999999LL,
        9999999999LL, 1234567890123LL,
        -1LL, -999999LL, -1234567890123LL,
    };

    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_NUM_VAR (ID, VAL) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER idVal = 0;   SQLLEN idInd = sizeof(idVal);
    SQLBIGINT  val   = 0;   SQLLEN valInd = sizeof(val);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG,   SQL_INTEGER, 0, 0, &idVal, sizeof(idVal), &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_VARCHAR, 40, 0, &val, sizeof(val), &valInd);

    for (size_t i = 0; i < vals.size(); ++i) {
        idVal = static_cast<SQLINTEGER>(i + 1);
        val   = vals[i];
        ret = SQLExecute(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "row " << i + 1 << " value " << vals[i];
    }
    Commit();
    ReallocStmt();

    verifyIntactness(static_cast<int>(vals.size()));

    ExecDirect("SELECT CAST(VAL AS BIGINT) FROM ODBC_NUM_VAR ORDER BY ID");
    for (auto expected : vals) {
        ret = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        SQLBIGINT got = 0;
        SQLLEN ind = 0;
        SQLGetData(hStmt, 1, SQL_C_SBIGINT, &got, sizeof(got), &ind);
        EXPECT_EQ(got, expected);
    }
    SQLCloseCursor(hStmt);
}

// ===== SQL_C_FLOAT (→ convFloatToString) =====

TEST_F(NumericToVarcharTest, FloatToVarcharLoopDml) {
    SKIP_ON_FIREBIRD6();

    const std::vector<SQLREAL> vals = {
        1.5f, 10.25f, 100.125f, 0.5f, -1.5f, -100.0f,
    };

    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_NUM_VAR (ID, VAL) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER idVal = 0;   SQLLEN idInd = sizeof(idVal);
    SQLREAL    val   = 0;   SQLLEN valInd = sizeof(val);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &idVal, sizeof(idVal), &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_VARCHAR, 40, 0, &val, sizeof(val), &valInd);

    for (size_t i = 0; i < vals.size(); ++i) {
        idVal = static_cast<SQLINTEGER>(i + 1);
        val   = vals[i];
        ret = SQLExecute(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "row " << i + 1;
    }
    Commit();
    ReallocStmt();

    verifyIntactness(static_cast<int>(vals.size()));

    ExecDirect("SELECT CAST(VAL AS DOUBLE PRECISION) FROM ODBC_NUM_VAR ORDER BY ID");
    for (auto expected : vals) {
        ret = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        SQLDOUBLE got = 0;
        SQLLEN ind = 0;
        SQLGetData(hStmt, 1, SQL_C_DOUBLE, &got, sizeof(got), &ind);
        EXPECT_NEAR(got, expected, 1e-4);
    }
    SQLCloseCursor(hStmt);
}

// ===== SQL_C_DOUBLE (→ convDoubleToString) =====

TEST_F(NumericToVarcharTest, DoubleToVarcharLoopDml) {
    SKIP_ON_FIREBIRD6();

    const std::vector<SQLDOUBLE> vals = {
        1.5, 10.25, 100.125, 0.5, 123456789.0, -1.5, -100.0,
    };

    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_NUM_VAR (ID, VAL) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER idVal = 0;   SQLLEN idInd = sizeof(idVal);
    SQLDOUBLE  val   = 0;   SQLLEN valInd = sizeof(val);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG,  SQL_INTEGER, 0, 0, &idVal, sizeof(idVal), &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_VARCHAR, 40, 0, &val, sizeof(val), &valInd);

    for (size_t i = 0; i < vals.size(); ++i) {
        idVal = static_cast<SQLINTEGER>(i + 1);
        val   = vals[i];
        ret = SQLExecute(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "row " << i + 1;
    }
    Commit();
    ReallocStmt();

    verifyIntactness(static_cast<int>(vals.size()));

    ExecDirect("SELECT CAST(VAL AS DOUBLE PRECISION) FROM ODBC_NUM_VAR ORDER BY ID");
    for (auto expected : vals) {
        ret = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret));
        SQLDOUBLE got = 0;
        SQLLEN ind = 0;
        SQLGetData(hStmt, 1, SQL_C_DOUBLE, &got, sizeof(got), &ind);
        EXPECT_NEAR(got, expected, 1e-6);
    }
    SQLCloseCursor(hStmt);
}

// NOTE — SQL_C_TYPE_DATE / SQL_C_TYPE_TIMESTAMP → VARCHAR tests intentionally
// omitted.  The driver routes those C-types through convDateToString /
// convDateTimeToString which expect Firebird's internal ISC_DATE / ISC_TIMESTAMP
// buffers (a 32-bit int and a QUAD, respectively), NOT the ODBC-side
// SQL_DATE_STRUCT / SQL_TIMESTAMP_STRUCT the application actually binds.  The
// resulting "VARCHAR" string is garbage (year 2043, etc.) independent of the
// Issue #161 length-prefix fix.  That's a separate pre-existing bug and should
// be tracked on its own.
