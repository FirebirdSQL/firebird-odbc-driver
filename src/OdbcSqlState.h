/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *
 *  Copyright (c) 2026 Firebird ODBC Driver Contributors
 *  All Rights Reserved.
 *
 *  OdbcSqlState.h — Comprehensive ISC→SQLSTATE and SQL Code→SQLSTATE mapping
 *
 *  This module provides:
 *    1. A comprehensive table of ODBC SQLSTATEs with both ODBC 3.x and 2.x strings
 *    2. Mapping from Firebird ISC error codes to SQLSTATE indices
 *    3. Mapping from legacy SQL error codes to SQLSTATE indices
 *    4. Version-aware SQLSTATE lookup (returns 2.x or 3.x string based on env setting)
 *
 *  References:
 *    - ODBC Appendix A: https://learn.microsoft.com/en-us/sql/odbc/reference/appendixes/appendix-a-odbc-error-codes
 *    - SQLSTATE Mappings: https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/sqlstate-mappings
 *    - Firebird ISC Error Codes: iberror.h
 *    - psqlodbc Statement_sqlstate[] pattern
 */

#pragma once

namespace OdbcJdbcLibrary {

/// Entry in the master SQLSTATE table.
/// Each entry carries both the ODBC 3.x and ODBC 2.x SQLSTATE strings.
/// When no 2.x equivalent exists, ver2State == ver3State.
struct SqlStateEntry {
    const char* ver3State;   // ODBC 3.x SQLSTATE (5 chars)
    const char* ver2State;   // ODBC 2.x SQLSTATE (5 chars), or same as ver3State
    const char* description; // Human-readable description
};

/// Master SQLSTATE table — index positions must be stable.
/// The listIscToSqlState[] and listSqlCodeToSqlState[] tables reference these by index.
///
/// ODBC 2.x ↔ 3.x mapping rules (from MS SQLSTATE Mappings doc):
///   HY000 ↔ S1000,  HY001 ↔ S1001,  HY003 ↔ S1003,  HY004 ↔ S1004,
///   HY008 ↔ S1008,  HY009 ↔ S1009,  HY010 ↔ S1010,  HY011 ↔ S1011,
///   HY012 ↔ S1012,  HY090 ↔ S1090,  HY091 ↔ S1091,  HY092 ↔ S1092,
///   HY096 ↔ S1096,  HY097 ↔ S1097,  HY098 ↔ S1098,  HY099 ↔ S1099,
///   HY100 ↔ S1100,  HY101 ↔ S1101,  HY103 ↔ S1103,  HY104 ↔ S1104,
///   HY105 ↔ S1105,  HY106 ↔ S1106,  HY107 ↔ S1107,  HY109 ↔ S1109,
///   HY110 ↔ S1110,  HY111 ↔ S1111,  HYC00 ↔ S1C00,  HYT00 ↔ S1T00,
///   42000 ↔ 37000,  42S01 ↔ S0001,  42S02 ↔ S0002,  42S11 ↔ S0011,
///   42S12 ↔ S0012,  42S21 ↔ S0021,  42S22 ↔ S0022,  07009 ↔ S1002
///
static const SqlStateEntry kSqlStates[] = {
    // Index   ODBC 3.x    ODBC 2.x    Description
    /*  0 */ { "01000",   "01000",   "General warning" },
    /*  1 */ { "01001",   "01S03",   "Cursor operation conflict" },
    /*  2 */ { "01002",   "01002",   "Disconnect error" },
    /*  3 */ { "01003",   "01003",   "NULL value eliminated in set function" },
    /*  4 */ { "01004",   "01004",   "String data, right truncated" },
    /*  5 */ { "01006",   "01006",   "Privilege not revoked" },
    /*  6 */ { "01007",   "01007",   "Privilege not granted" },
    /*  7 */ { "01S00",   "01S00",   "Invalid connection string attribute" },
    /*  8 */ { "01S01",   "01S01",   "Error in row" },
    /*  9 */ { "01S02",   "01S02",   "Option value changed" },
    /* 10 */ { "01S06",   "01S06",   "Attempt to fetch before the result set returned the first rowset" },
    /* 11 */ { "01S07",   "01S07",   "Fractional truncation" },
    /* 12 */ { "01S08",   "01S08",   "Error saving File DSN" },
    /* 13 */ { "01S09",   "01S09",   "Invalid keyword" },
    /* 14 */ { "07001",   "07001",   "Wrong number of parameters" },
    /* 15 */ { "07002",   "07002",   "COUNT field incorrect" },
    /* 16 */ { "07005",   "24000",   "Prepared statement not a cursor-specification" },
    /* 17 */ { "07006",   "07006",   "Restricted data type attribute violation" },
    /* 18 */ { "07009",   "S1002",   "Invalid descriptor index" },
    /* 19 */ { "07S01",   "07S01",   "Invalid use of default parameter" },
    /* 20 */ { "08001",   "08001",   "Client unable to establish connection" },
    /* 21 */ { "08002",   "08002",   "Connection name in use" },
    /* 22 */ { "08003",   "08003",   "Connection does not exist" },
    /* 23 */ { "08004",   "08004",   "Server rejected the connection" },
    /* 24 */ { "08007",   "08007",   "Connection failure during transaction" },
    /* 25 */ { "08S01",   "08S01",   "Communication link failure" },
    /* 26 */ { "21S01",   "21S01",   "Insert value list does not match column list" },
    /* 27 */ { "21S02",   "21S02",   "Degree of derived table does not match column list" },
    /* 28 */ { "22001",   "22001",   "String data, right truncated" },
    /* 29 */ { "22002",   "22002",   "Indicator variable required but not supplied" },
    /* 30 */ { "22003",   "22003",   "Numeric value out of range" },
    /* 31 */ { "22007",   "22008",   "Invalid datetime format" },
    /* 32 */ { "22008",   "22008",   "Datetime field overflow" },
    /* 33 */ { "22012",   "22012",   "Division by zero" },
    /* 34 */ { "22015",   "22015",   "Interval field overflow" },
    /* 35 */ { "22018",   "22005",   "Invalid character value for cast specification" },
    /* 36 */ { "22019",   "22019",   "Invalid escape character" },
    /* 37 */ { "22025",   "22025",   "Invalid escape sequence" },
    /* 38 */ { "22026",   "22026",   "String data, length mismatch" },
    /* 39 */ { "23000",   "23000",   "Integrity constraint violation" },
    /* 40 */ { "24000",   "24000",   "Invalid cursor state" },
    /* 41 */ { "25000",   "25000",   "Invalid transaction state" },
    /* 42 */ { "25S01",   "25S01",   "Transaction state" },
    /* 43 */ { "25S02",   "25S02",   "Transaction is still active" },
    /* 44 */ { "25S03",   "25S03",   "Transaction is rolled back" },
    /* 45 */ { "28000",   "28000",   "Invalid authorization specification" },
    /* 46 */ { "34000",   "34000",   "Invalid cursor name" },
    /* 47 */ { "3C000",   "3C000",   "Duplicate cursor name" },
    /* 48 */ { "3D000",   "3D000",   "Invalid catalog name" },
    /* 49 */ { "3F000",   "3F000",   "Invalid schema name" },
    /* 50 */ { "40001",   "40001",   "Serialization failure" },
    /* 51 */ { "40002",   "40002",   "Integrity constraint violation" },
    /* 52 */ { "40003",   "40003",   "Statement completion unknown" },
    /* 53 */ { "42000",   "37000",   "Syntax error or access violation" },
    /* 54 */ { "42S01",   "S0001",   "Base table or view already exists" },
    /* 55 */ { "42S02",   "S0002",   "Base table or view not found" },
    /* 56 */ { "42S11",   "S0011",   "Index already exists" },
    /* 57 */ { "42S12",   "S0012",   "Index not found" },
    /* 58 */ { "42S21",   "S0021",   "Column already exists" },
    /* 59 */ { "42S22",   "S0022",   "Column not found" },
    /* 60 */ { "44000",   "44000",   "WITH CHECK OPTION violation" },
    /* 61 */ { "HY000",   "S1000",   "General error" },
    /* 62 */ { "HY001",   "S1001",   "Memory allocation error" },
    /* 63 */ { "HY003",   "S1003",   "Invalid application buffer type" },
    /* 64 */ { "HY004",   "S1004",   "Invalid SQL data type" },
    /* 65 */ { "HY007",   "S1010",   "Associated statement is not prepared" },
    /* 66 */ { "HY008",   "S1008",   "Operation canceled" },
    /* 67 */ { "HY009",   "S1009",   "Invalid use of null pointer" },
    /* 68 */ { "HY010",   "S1010",   "Function sequence error" },
    /* 69 */ { "HY011",   "S1011",   "Attribute cannot be set now" },
    /* 70 */ { "HY012",   "S1012",   "Invalid transaction operation code" },
    /* 71 */ { "HY013",   "S1000",   "Memory management error" },
    /* 72 */ { "HY014",   "S1000",   "Limit on the number of handles exceeded" },
    /* 73 */ { "HY015",   "S1000",   "No cursor name available" },
    /* 74 */ { "HY016",   "S1000",   "Cannot modify an implementation row descriptor" },
    /* 75 */ { "HY017",   "S1000",   "Invalid use of an automatically allocated descriptor handle" },
    /* 76 */ { "HY018",   "70100",   "Server declined cancel request" },
    /* 77 */ { "HY019",   "22003",   "Non-character and non-binary data sent in pieces" },
    /* 78 */ { "HY020",   "S1000",   "Attempt to concatenate a null value" },
    /* 79 */ { "HY021",   "S1000",   "Inconsistent descriptor information" },
    /* 80 */ { "HY024",   "S1009",   "Invalid attribute value" },
    /* 81 */ { "HY090",   "S1090",   "Invalid string or buffer length" },
    /* 82 */ { "HY091",   "S1091",   "Invalid descriptor field identifier" },
    /* 83 */ { "HY092",   "S1092",   "Invalid attribute/option identifier" },
    /* 84 */ { "HY095",   "S1000",   "Function type out of range" },
    /* 85 */ { "HY096",   "S1096",   "Invalid information type" },
    /* 86 */ { "HY097",   "S1097",   "Column type out of range" },
    /* 87 */ { "HY098",   "S1098",   "Scope type out of range" },
    /* 88 */ { "HY099",   "S1099",   "Nullable type out of range" },
    /* 89 */ { "HY100",   "S1100",   "Uniqueness option type out of range" },
    /* 90 */ { "HY101",   "S1101",   "Accuracy option type out of range" },
    /* 91 */ { "HY103",   "S1103",   "Invalid retrieval code" },
    /* 92 */ { "HY104",   "S1104",   "Invalid precision or scale value" },
    /* 93 */ { "HY105",   "S1105",   "Invalid parameter type" },
    /* 94 */ { "HY106",   "S1106",   "Fetch type out of range" },
    /* 95 */ { "HY107",   "S1107",   "Row value out of range" },
    /* 96 */ { "HY109",   "S1109",   "Invalid cursor position" },
    /* 97 */ { "HY110",   "S1110",   "Invalid driver completion" },
    /* 98 */ { "HY111",   "S1111",   "Invalid bookmark value" },
    /* 99 */ { "HYC00",   "S1C00",   "Optional feature not implemented" },
    /*100 */ { "HYT00",   "S1T00",   "Timeout expired" },
    /*101 */ { "HYT01",   "S1T00",   "Connection timeout expired" },
    /*102 */ { "IM001",   "IM001",   "Driver does not support this function" },
    /*103 */ { "IM002",   "IM002",   "Data source name not found and no default driver specified" },
    /*104 */ { "IM003",   "IM003",   "Specified driver could not be loaded" },
    /*105 */ { "IM004",   "IM004",   "Driver's SQLAllocHandle on SQL_HANDLE_ENV failed" },
    /*106 */ { "IM005",   "IM005",   "Driver's SQLAllocHandle on SQL_HANDLE_DBC failed" },
    /*107 */ { "IM006",   "IM006",   "Driver's SQLSetConnectAttr failed" },
    /*108 */ { "IM007",   "IM007",   "No data source or driver specified; dialog prohibited" },
    /*109 */ { "IM008",   "IM008",   "Dialog failed" },
    /*110 */ { "IM009",   "IM009",   "Unable to load translation DLL" },
    /*111 */ { "IM010",   "IM010",   "Data source name too long" },
    /*112 */ { "IM011",   "IM011",   "Driver name too long" },
    /*113 */ { "IM012",   "IM012",   "DRIVER keyword syntax error" },
    /*114 */ { "IM013",   "IM013",   "Trace file error" },
    /*115 */ { "IM014",   "IM014",   "Invalid name of File DSN" },
    /*116 */ { "IM015",   "IM015",   "Corrupt file data source" },
    // Extended entries for Firebird-specific mappings
    /*117 */ { "08006",   "08S01",   "Connection failure" },
    /*118 */ { "22000",   "22000",   "Data exception" },
    /*119 */ { "27000",   "27000",   "Triggered data change violation" },
    /*120 */ { "54000",   "54000",   "Program limit exceeded" },
};

static const int kSqlStatesCount = sizeof(kSqlStates) / sizeof(kSqlStates[0]);

// ---------------------------------------------------------------------------
// ISC error code → SQLSTATE index mapping
//
// This is the comprehensive mapping from Firebird ISC status codes to indices
// in the kSqlStates[] table above. Previously only 19 codes were mapped.
// This table now covers all commonly encountered Firebird errors.
// ---------------------------------------------------------------------------

struct IscToSqlStateEntry {
    long         iscCode;       // Firebird ISC encoded error code (e.g., 335544336)
    int          sqlStateIndex; // Index into kSqlStates[]
};

/// Comprehensive ISC error code → SQLSTATE index mapping.
/// Sorted by iscCode for binary search lookup.
static const IscToSqlStateEntry kIscToSqlState[] = {
    // isc_arith_except (335544321) — arithmetic exception → 22000 Data exception
    { 335544321L,  118 },

    // isc_bad_db_handle (335544324) — invalid database handle → 08003 Connection does not exist
    { 335544324L,   22 },

    // isc_bad_dpb_content (335544325) — bad parameters on attach → HY000 General error
    { 335544325L,   61 },

    // isc_bad_req_handle (335544327) — invalid request handle → HY010 Function sequence error
    { 335544327L,   68 },

    // isc_bad_tpb_content (335544330) — invalid transaction param → 25000 Invalid transaction state
    { 335544330L,   41 },

    // isc_bad_trans_handle (335544332) — invalid transaction handle → 25000 Invalid transaction state
    { 335544332L,   41 },

    // isc_bug_check (335544333) — internal Firebird consistency check → HY000 General error
    { 335544333L,   61 },

    // isc_convert_error (335544334) — conversion error → 22018 Invalid char value for cast
    { 335544334L,   35 },

    // isc_db_corrupt (335544335) — database file appears corrupt → HY000 General error
    { 335544335L,   61 },

    // isc_deadlock (335544336) — deadlock → 40001 Serialization failure
    { 335544336L,   50 },

    // isc_excess_trans (335544337) — too many concurrent transactions → HY000 General error
    { 335544337L,   61 },

    // isc_from_no_match (335544338) — no match for FROM → 42000 Syntax error or access violation
    { 335544338L,   53 },

    // isc_infinap (335544339) — info-type not appropriate → HY096 Invalid information type
    { 335544339L,   85 },

    // isc_infona (335544340) — no info of this type available → HY096 Invalid information type
    { 335544340L,   85 },

    // isc_infunk (335544341) — unknown info item → HY096 Invalid information type
    { 335544341L,   85 },

    // isc_integ_fail (335544342) — action cancelled by trigger → 27000 Triggered data change violation
    { 335544342L,  119 },

    // isc_no_cur_rec (335544348) — no current record for fetch → 22000 Data exception
    { 335544348L,  118 },

    // isc_lock_conflict (335544345) — lock conflict → 40001 Serialization failure
    { 335544345L,   50 },

    // isc_metadata_corrupt (335544346) — corrupt system table → HY000 General error
    { 335544346L,   61 },

    // isc_not_valid (335544347) — validation error → 23000 Integrity constraint violation
    { 335544347L,   39 },

    // isc_no_dup (335544349) — duplicate value in unique index → 23000 Integrity constraint violation
    { 335544349L,   39 },

    // isc_no_finish (335544350) — program attempted to exit without finishing database → HY000
    { 335544350L,   61 },

    // isc_no_meta_update (335544351) — unsuccessful metadata update → 42000 Syntax error or access violation
    { 335544351L,   53 },

    // isc_no_priv (335544352) — no permission for this operation → 42000 Syntax error or access violation
    { 335544352L,   53 },

    // isc_no_recon (335544353) — transaction is not in limbo → 25000 Invalid transaction state
    { 335544353L,   41 },

    // isc_no_record (335544354) — no record for this fetch → 22000 Data exception
    { 335544354L,  118 },

    // isc_segment (335544356) — segment buffer too short → 01004 String data, right truncated
    { 335544356L,    4 },

    // isc_segstr_eof (335544357) — attempted read past end of blob segment → HY000
    { 335544357L,   61 },

    // isc_shutdown (335544360) — database shutdown → 08001 Client unable to establish connection
    { 335544360L,   20 },

    // isc_stream_eof (335544367) — end of stream → HY000
    { 335544367L,   61 },

    // isc_unavailable (335544375) — unavailable database → 08001 Client unable to establish connection
    { 335544375L,   20 },

    // isc_unres_rel (335544379) — table not defined → 42S02 Base table or view not found
    { 335544379L,   55 },

    // isc_imp_exc (335544381) — implementation limit exceeded → 54000 Program limit exceeded
    { 335544381L,  120 },

    // isc_random (335544382) — general random error → HY000
    { 335544382L,   61 },

    // isc_tra_state (335544385) — transaction state invalid → 25000 Invalid transaction state
    { 335544385L,   41 },

    // isc_no_segstr_close (335544393) — blob not properly closed → HY000
    { 335544393L,   61 },

    // isc_wrong_ods (335544394) — unsupported database ODS → 08001 Client unable to establish connection
    { 335544394L,   20 },

    // isc_connect_reject (335544421) — connection rejected → 08004 Server rejected the connection
    { 335544421L,   23 },

    // isc_no_lock_mgr (335544424) — no lock manager → 08001 Client unable to establish connection
    { 335544424L,   20 },

    // isc_ctxinuse (335544433) — context already in use → HY000
    { 335544433L,   61 },

    // isc_ctxnotdef (335544434) — context not defined → HY000
    { 335544434L,   61 },

    // isc_datnotsup (335544435) — data operation not supported → HYC00 Optional feature not implemented
    { 335544435L,   99 },

    // isc_badmsgnum (335544436) — bad message number → HY000
    { 335544436L,   61 },

    // isc_badparnum (335544437) — bad parameter number → 07009 Invalid descriptor index
    { 335544437L,   18 },

    // isc_tra_no_trans (335544445) — no transaction yet → 25000 Invalid transaction state
    { 335544445L,   41 },

    // isc_gennotdef (335544463) — generator/sequence not defined → 42000 Syntax error or access violation
    { 335544463L,   53 },

    // isc_foreign_key (335544466) — FOREIGN KEY constraint violation → 23000 Integrity constraint violation
    { 335544466L,   39 },

    // isc_login (335544472) — login failure → 28000 Invalid authorization specification
    { 335544472L,   45 },

    // isc_tra_in_limbo (335544480) — transaction in limbo → 25000 Invalid transaction state
    { 335544480L,   41 },

    // isc_max_idx (335544494) — maximum indexes per table exceeded → 54000 Program limit exceeded
    { 335544494L,  120 },

    // isc_wrong_ods (repeated, different sub-error context)
    // isc_idx_create_err (335544497) — cannot create index → 42000
    { 335544497L,   53 },

    // isc_idx_key_err (335544502) — key size exceeds implementation restriction → 54000
    { 335544502L,  120 },

    // isc_check_constraint (335544558) — CHECK constraint violation → 23000 Integrity constraint violation
    { 335544558L,   39 },

    // isc_dsql_error (335544569) — DSQL error — INTENTIONALLY OMITTED
    // This is a generic wrapper code that covers syntax errors, table-not-found,
    // column-not-found, etc. The specific sub-error codes and SQL codes provide
    // better SQLSTATE mapping, so we let them take priority via fallback.

    // isc_dsql_command_err (335544570) — Invalid command → 42000 Syntax error or access violation
    { 335544570L,   53 },

    // isc_dsql_constant_err (335544571) — data type for constant unknown → 42000
    { 335544571L,   53 },

    // isc_dsql_cursor_err (335544572) — cursor unknown → 34000 Invalid cursor name
    { 335544572L,   46 },

    // isc_dsql_datatype_err (335544573) — data type unknown → HY004 Invalid SQL data type
    { 335544573L,   64 },

    // isc_dsql_decl_err (335544574) — declaration error → 42000
    { 335544574L,   53 },

    // isc_dsql_cursor_update_err (335544575) — cursor not updatable → 24000 Invalid cursor state
    { 335544575L,   40 },

    // isc_dsql_cursor_open_err (335544576) — attempt to open an open cursor → 24000 Invalid cursor state
    { 335544576L,   40 },

    // isc_dsql_cursor_close_err (335544577) — attempt to close a closed cursor → 24000 Invalid cursor state
    { 335544577L,   40 },

    // isc_dsql_field_err (335544578) — column unknown → 42S22 Column not found
    { 335544578L,   59 },

    // isc_dsql_internal_err (335544579) — internal error in DSQL → HY000
    { 335544579L,   61 },

    // isc_dsql_relation_err (335544580) — table unknown → 42S02 Base table or view not found
    { 335544580L,   55 },

    // isc_dsql_procedure_err (335544581) — procedure unknown → 42000 Syntax error or access violation
    { 335544581L,   53 },

    // isc_dsql_request_err (335544582) — request unknown → HY000
    { 335544582L,   61 },

    // isc_dsql_sqlda_err (335544583) — SQLDA error → 07002 COUNT field incorrect
    { 335544583L,   15 },

    // isc_dsql_var_count_err (335544584) — count of read-write columns does not equal count of values
    // → 21S01 Insert value list does not match column list
    { 335544584L,   26 },

    // isc_dsql_stmt_handle (335544585) — invalid statement handle → HY010 Function sequence error
    { 335544585L,   68 },

    // isc_dsql_function_err (335544586) — function unknown → 42000
    { 335544586L,   53 },

    // isc_dsql_blob_err (335544587) — column not a blob → 07006 Restricted data type attribute violation
    { 335544587L,   17 },

    // isc_dsql_ambiguous_field_name (335544594) — ambiguous column reference → 42000
    { 335544594L,   53 },

    // isc_dsql_duplicate_spec (335544597) — duplicate specification → 42000
    { 335544597L,   53 },

    // isc_dsql_field_ref (335544601) — invalid field reference → 42S22 Column not found
    { 335544601L,   59 },

    // isc_dsql_relation_lock (335544610) — table lock → 40001 Serialization failure (lock-related)
    { 335544610L,   50 },

    // isc_dsql_token_unk_err (335544634) — token unknown → 42000 Syntax error or access violation
    { 335544634L,   53 },

    // isc_dsql_no_dup_name (335544638) — no duplicate name → 42000
    { 335544638L,   53 },

    // isc_conn_lost (335544648) — connection lost to pipe server → 08S01 Communication link failure
    { 335544648L,   25 },

    // isc_dsql_col_bin_not_found (335544649) — column not found in context → 42S22
    { 335544649L,   59 },

    // isc_unique_key_violation (335544665) — UNIQUE KEY or PRIMARY KEY violation → 23000
    { 335544665L,   39 },

    // isc_no_delete (335544667) — cannot delete → 42000
    { 335544667L,   53 },

    // isc_no_update (335544668) — cannot update → 42000
    { 335544668L,   53 },

    // isc_stack_trace (335544669) — stack trace follows → HY000 (informational, carries trace)
    { 335544669L,   61 },

    // isc_except2 (335544683) — exception → HY000
    { 335544683L,   61 },

    // isc_malformed_string (335544686) — malformed string → 22018 Invalid character value for cast
    { 335544686L,   35 },

    // isc_command_end_err2 (335544692) — unexpected end of command → 42000
    { 335544692L,   53 },

    // isc_network_error (335544721) — unable to complete network request → 08001 
    { 335544721L,   20 },

    // isc_net_connect_err (335544722) — failed to establish connection → 08001
    { 335544722L,   20 },

    // isc_net_connect_listen_err (335544723) — error while listening for connection → 08001
    { 335544723L,   20 },

    // isc_net_event_connect_err (335544724) — failed to establish event connection → 08001
    { 335544724L,   20 },

    // isc_net_event_listen_err (335544725) — error while listening for event → 08001
    { 335544725L,   20 },

    // isc_net_read_err (335544726) — error reading data from connection → 08S01
    { 335544726L,   25 },

    // isc_net_write_err (335544727) — error writing data to connection → 08S01
    { 335544727L,   25 },

    // isc_net_server_shutdown (335544741) — server shutdown or connection lost → 08S01
    { 335544741L,   25 },

    // isc_max_att_exceeded (335544744) — maximum user count exceeded → 08004
    { 335544744L,   23 },

    // isc_arith_except_overflow (335544779) — integer overflow → 22003 Numeric value out of range
    { 335544779L,   30 },

    // isc_string_truncation (335544914) — string right truncation → 22001
    { 335544914L,   28 },

    // isc_numeric_out_of_range (335544916) — numeric value out of range → 22003
    { 335544916L,   30 },

    // isc_cancelled (335544794) — operation cancelled → HY008
    { 335544794L,   66 },

    // isc_too_many_handles (335544804) — too many open handles → HY014 Limit on the number of handles exceeded
    { 335544804L,   72 },

    // isc_dsql_agg_column_err — cannot use column in both aggregate and non-aggregate → 42000
    { 335544811L,   53 },

    // isc_dsql_agg_having_err — invalid aggregate reference in HAVING → 42000
    { 335544812L,   53 },

    // isc_dsql_agg_nested_err — nested aggregate functions → 42000
    { 335544813L,   53 },

    // isc_dsql_table_not_found — table not found → 42S02
    { 335544817L,   55 },

    // isc_null_value_no_ind — null value with no indicator → 22002
    // Indicator variable required but not supplied
    { 335544839L,   29 },

    // isc_datetime_range (335544841) — datetime out of range → 22008 Datetime field overflow
    { 335544841L,   32 },

    // isc_wrong_num_parameters — wrong number of parameters → 07001
    { 335544849L,   14 },

    // isc_division_by_zero — division by zero → 22012
    { 335544778L,   33 },

    // Firebird 3.0+ errors

    // isc_login_same_as_role_name (335544851) — cannot use role name as user → 28000
    { 335544851L,   45 },

    // isc_att_shutdown (335544856) — connection shutdown → 08S01 Communication link failure
    { 335544856L,   25 },

    // isc_blobtoobig (335544862) — blob size exceeds implementation limit → 54000
    { 335544862L,  120 },

    // isc_rec_in_limbo (335544863) — record in limbo → 40003 Statement completion unknown
    { 335544863L,   52 },

    // Firebird 4.0+ errors

    // isc_decfloat_divide_by_zero — DECFLOAT division by zero → 22012
    { 335545064L,   33 },

    // isc_decfloat_overflow — DECFLOAT overflow → 22003
    { 335545065L,   30 },

    // isc_decfloat_invalid_operation — DECFLOAT invalid operation → 22000
    { 335545066L,  118 },

    // isc_too_big_blr (335545079) — BLR too big → 54000
    { 335545079L,  120 },
};

static const int kIscToSqlStateCount = sizeof(kIscToSqlState) / sizeof(kIscToSqlState[0]);


// ---------------------------------------------------------------------------
// SQL error code → SQLSTATE index mapping
//
// These are the legacy SQL codes from Firebird (negative integers).
// Previously only 3 codes were mapped (-1 → 42000, -204 → 42S02, -913 → 40001).
// This table now provides comprehensive coverage.
// ---------------------------------------------------------------------------

struct SqlCodeToSqlStateEntry {
    int sqlCode;       // Firebird SQL error code (negative)
    int sqlStateIndex; // Index into kSqlStates[]
};

/// Comprehensive SQL error code → SQLSTATE index mapping.
/// Sorted by sqlCode (ascending, i.e., most negative first) for binary search.
static const SqlCodeToSqlStateEntry kSqlCodeToSqlState[] = {
    // SQL Code  → SQLSTATE index  |  Meaning
    { -924,  20 },  // 08001 — Connection/attachment error
    { -923,  23 },  // 08004 — Connection rejected
    { -922,  61 },  // HY000 — File is not a valid database
    { -913,  50 },  // 40001 — Deadlock / lock conflict
    { -911,  50 },  // 40001 — Record in use (lock conflict variant)
    { -909,  61 },  // HY000 — Drop database not supported
    { -906,  23 },  // 08004 — Max attachments exceeded
    { -904,  61 },  // HY000 — Unavailable/unimplemented (catch-all)
    { -902,  25 },  // 08S01 — Communication link failure / serious error
    { -901,  61 },  // HY000 — Misc runtime error
    { -842,  32 },  // 22008 — Datetime out of range
    { -841,  32 },  // 22008 — Datetime range exceeded
    { -840,  35 },  // 22018 — Conversion error on date/time
    { -838,  35 },  // 22018 — Conversion error
    { -836,  35 },  // 22018 — Conversion error
    { -834,  35 },  // 22018 — Conversion error
    { -833,  35 },  // 22018 — Conversion error
    { -831,  35 },  // 22018 — Conversion error
    { -829,  35 },  // 22018 — Conversion error
    { -828,  35 },  // 22018 — Conversion error
    { -827,  35 },  // 22018 — Conversion error
    { -826,  35 },  // 22018 — Conversion error
    { -825,  35 },  // 22018 — Conversion error
    { -824,  35 },  // 22018 — Conversion error
    { -823,  35 },  // 22018 — Conversion error
    { -820,  35 },  // 22018 — Conversion error
    { -817,  99 },  // HYC00 — Optional feature not implemented
    { -816,  99 },  // HYC00 — Optional feature not implemented
    { -811,  18 },  // 07009 — Multiple rows returned from singleton select
    { -810,  15 },  // 07002 — COUNT field incorrect
    { -809,  15 },  // 07002 — COUNT field incorrect
    { -808,  15 },  // 07002 — SQLDA missing or incorrect
    { -807,  15 },  // 07002 — COUNT field incorrect
    { -806,  64 },  // HY004 — Invalid SQL data type
    { -804,  15 },  // 07002 — SQLDA / parameter count mismatch
    { -803,  39 },  // 23000 — Duplicate key value (unique constraint)
    { -802,  30 },  // 22003 — Numeric value out of range / arithmetic exception
    { -694,  53 },  // 42000 — Exception in procedure/trigger
    { -693,  53 },  // 42000 — Exception in procedure/trigger
    { -692,  53 },  // 42000 — Exception in procedure/trigger
    { -691,  53 },  // 42000 — Exception in procedure/trigger
    { -690,  53 },  // 42000 — Exception in procedure/trigger
    { -689,  53 },  // 42000 — Maximum recursion depth exceeded
    { -685,  53 },  // 42000 — Incompatible trigger type
    { -677,  53 },  // 42000 — External function error
    { -664,  53 },  // 42000 — Array declaration error
    { -663,  53 },  // 42000 — Invalid array reference
    { -660,  53 },  // 42000 — Blob filter error
    { -637,  53 },  // 42000 — Duplicate specification
    { -625,  39 },  // 23000 — Validation error (NOT NULL, etc.)
    { -618,  53 },  // 42000 — DSQL error (misc)
    { -617,  53 },  // 42000 — DSQL error (misc)
    { -616,  53 },  // 42000 — DSQL error (misc)
    { -615,  53 },  // 42000 — DSQL error (misc)
    { -612,  53 },  // 42000 — DSQL error
    { -607,  53 },  // 42000 — Unsuccessful metadata update
    { -605,  54 },  // 42S01 — Object already exists (table/view)
    { -604,  53 },  // 42000 — Data type error
    { -601,  53 },  // 42000 — Invalid DSQL element
    { -600,  53 },  // 42000 — Invalid DSQL element
    { -599,  53 },  // 42000 — Invalid DSQL element
    { -598,  53 },  // 42000 — Invalid DSQL element
    { -597,  53 },  // 42000 — Invalid DSQL element
    { -596,  53 },  // 42000 — Invalid DSQL element
    { -595,  53 },  // 42000 — Invalid DSQL element
    { -553,  53 },  // 42000 — No permission (DDL)
    { -552,  53 },  // 42000 — No permission (DML)
    { -551,  53 },  // 42000 — No permission
    { -532,  39 },  // 23000 — Foreign key violation (update)
    { -531,  39 },  // 23000 — Foreign key violation (delete)
    { -530,  39 },  // 23000 — Foreign key violation (insert)
    { -519,  40 },  // 24000 — Invalid cursor state
    { -518,  40 },  // 24000 — Invalid cursor state
    { -510,  40 },  // 24000 — Invalid cursor state / no current record
    { -508, 118 },  // 22000 — No record for fetch
    { -504,  46 },  // 34000 — Invalid cursor name
    { -502,  40 },  // 24000 — Invalid cursor state (cursor not open)
    { -501,  40 },  // 24000 — Invalid cursor state (cursor already open)
    { -413,  35 },  // 22018 — Conversion error
    { -407,  29 },  // 22002 — Indicator variable required but not supplied (null value)
    { -406,  61 },  // HY000 — Dynamic SQLDA mismatch
    { -402,  14 },  // 07001 — Wrong number of parameters
    { -401,  14 },  // 07001 — Wrong number of parameters
    { -383,  53 },  // 42000 — Error in stored procedure
    { -315,  53 },  // 42000 — Ambiguous field/column reference
    { -314,  53 },  // 42000 — Field/column reference error
    { -313,  14 },  // 07001 — Wrong number of parameters
    { -297,  39 },  // 23000 — Check constraint violation
    { -296,  53 },  // 42000 — Not in list of valid transactions
    { -295,  53 },  // 42000 — Engine / internal error
    { -294,  53 },  // 42000 — Engine / internal error
    { -293,  53 },  // 42000 — Engine / internal error
    { -292,  53 },  // 42000 — Engine / internal error
    { -291,  53 },  // 42000 — Engine / internal error
    { -284,  53 },  // 42000 — DDL / metadata error
    { -283,  53 },  // 42000 — DDL / metadata error
    { -282,  53 },  // 42000 — DDL / metadata error
    { -281,  53 },  // 42000 — DDL / metadata error
    { -261,  53 },  // 42000 — Domain/field error
    { -260,  53 },  // 42000 — Domain/field error
    { -259,  53 },  // 42000 — Domain/field error
    { -258,  53 },  // 42000 — Domain/field error
    { -257,  53 },  // 42000 — Domain/field error
    { -255,  53 },  // 42000 — Domain/field error
    { -254,  53 },  // 42000 — Domain/field error
    { -253,  53 },  // 42000 — Domain/field error
    { -252,  53 },  // 42000 — Domain/field error
    { -251,  53 },  // 42000 — Domain/field error
    { -250,  53 },  // 42000 — Domain/field error
    { -249,  53 },  // 42000 — Domain/field error
    { -248,  53 },  // 42000 — Domain/field error
    { -247,  53 },  // 42000 — Domain/field error
    { -246,  53 },  // 42000 — Domain/field error
    { -245,  53 },  // 42000 — Domain/field error
    { -244,  53 },  // 42000 — Domain/field error
    { -243,  53 },  // 42000 — Domain/field error
    { -242,  53 },  // 42000 — Domain/field error
    { -241,  53 },  // 42000 — Domain/field error
    { -240,  53 },  // 42000 — Domain/field error
    { -239,  53 },  // 42000 — Domain/field error
    { -238,  53 },  // 42000 — Domain/field error
    { -237,  53 },  // 42000 — Domain/field error
    { -236,  53 },  // 42000 — Domain/field error
    { -235,  53 },  // 42000 — Domain/field error
    { -234,  53 },  // 42000 — Domain/field error
    { -233,  53 },  // 42000 — Domain/field error
    { -232,  53 },  // 42000 — Domain/field error
    { -231,  53 },  // 42000 — Domain/field error
    { -230,  53 },  // 42000 — Domain/field error
    { -219,  53 },  // 42000 — Field/column not found in context
    { -208,  53 },  // 42000 — Invalid ORDER BY clause
    { -206,  59 },  // 42S22 — Column not found
    { -205,  53 },  // 42000 — Unknown identifier
    { -204,  55 },  // 42S02 — Table/view/procedure not found
    { -203,  53 },  // 42000 — Ambiguous column reference
    { -172,  53 },  // 42000 — DSQL syntax error
    { -171,  64 },  // HY004 — Invalid data type
    { -170,  53 },  // 42000 — DSQL error
    { -162,  53 },  // 42000 — Command unknown
    { -158,  53 },  // 42000 — Blob error
    { -157,  53 },  // 42000 — Blob error
    { -155,  53 },  // 42000 — Blob filter not found
    { -151,  26 },  // 21S01 — Column count mismatch in INSERT
    { -150,  53 },  // 42000 — Relation/table error
    { -105,  53 },  // 42000 — Syntax error
    { -104,  53 },  // 42000 — Token unknown / syntax error
    { -103,  53 },  // 42000 — Data type error
    {  -85,  53 },  // 42000 — DSQL error
    {  -84,  53 },  // 42000 — DSQL error
    {   -1,  53 },  // 42000 — Syntax error (general)
};

static const int kSqlCodeToSqlStateCount = sizeof(kSqlCodeToSqlState) / sizeof(kSqlCodeToSqlState[0]);


// ---------------------------------------------------------------------------
// Lookup functions
// ---------------------------------------------------------------------------

/// Look up the SQLSTATE index for an ISC error code using linear search.
/// Returns the index into kSqlStates[], or -1 if not found.
static inline int findSqlStateByIscCode(long iscCode)
{
    // Linear search (table is ~100 entries, fast enough)
    for (int i = 0; i < kIscToSqlStateCount; ++i) {
        if (kIscToSqlState[i].iscCode == iscCode)
            return kIscToSqlState[i].sqlStateIndex;
    }
    return -1;
}

/// Look up the SQLSTATE index for a legacy SQL error code using linear search.
/// Returns the index into kSqlStates[], or -1 if not found.
static inline int findSqlStateBySqlCode(int sqlCode)
{
    for (int i = 0; i < kSqlCodeToSqlStateCount; ++i) {
        if (kSqlCodeToSqlState[i].sqlCode == sqlCode)
            return kSqlCodeToSqlState[i].sqlStateIndex;
    }
    return -1;
}

/// Look up a SQLSTATE index by 3.x SQLSTATE string.
/// Returns the index into kSqlStates[], or -1 if not found.
static inline int findSqlStateByString(const char* ver3State)
{
    if (!ver3State)
        return -1;
    for (int i = 0; i < kSqlStatesCount; ++i) {
        if (strncmp(kSqlStates[i].ver3State, ver3State, 5) == 0)
            return i;
    }
    return -1;
}

/// Get the version-appropriate SQLSTATE string.
/// @param index     Index into kSqlStates[]
/// @param useOdbc3  true for ODBC 3.x, false for ODBC 2.x
/// @return The 5-character SQLSTATE string, or "HY000" if index is invalid.
static inline const char* getSqlStateString(int index, bool useOdbc3)
{
    if (index < 0 || index >= kSqlStatesCount)
        return useOdbc3 ? "HY000" : "S1000";
    return useOdbc3 ? kSqlStates[index].ver3State : kSqlStates[index].ver2State;
}

/// Resolve the best SQLSTATE for a given error, checking ISC code first, then SQL code,
/// then falling back to the provided default SQLSTATE string.
/// @param iscCode    Firebird ISC error code (0 if not available)
/// @param sqlCode    Legacy SQL error code (0 if not available)
/// @param defaultState  Fallback SQLSTATE string (e.g., "HY000")
/// @param useOdbc3   true for ODBC 3.x, false for ODBC 2.x
/// @param outState   Output buffer (must hold at least 6 chars)
static inline void resolveSqlState(long iscCode, int sqlCode, const char* defaultState,
                                   bool useOdbc3, char* outState)
{
    int index = -1;

    // Priority 1: ISC error code
    if (iscCode != 0)
        index = findSqlStateByIscCode(iscCode);

    // Priority 2: Legacy SQL error code
    if (index < 0 && sqlCode != 0)
        index = findSqlStateBySqlCode(sqlCode);

    // Priority 3: Default state string — try to find its versioned equivalent
    if (index < 0 && defaultState) {
        index = findSqlStateByString(defaultState);
        if (index >= 0) {
            // Found: return the version-appropriate string
            const char* result = getSqlStateString(index, useOdbc3);
            memcpy(outState, result, 5);
            outState[5] = '\0';
            return;
        }
        // Not found in table: use defaultState as-is
        memcpy(outState, defaultState, 5);
        outState[5] = '\0';
        return;
    }

    if (index >= 0) {
        const char* result = getSqlStateString(index, useOdbc3);
        memcpy(outState, result, 5);
        outState[5] = '\0';
    } else {
        // Ultimate fallback
        const char* fallback = useOdbc3 ? "HY000" : "S1000";
        memcpy(outState, fallback, 5);
        outState[5] = '\0';
    }
}

} // namespace OdbcJdbcLibrary
