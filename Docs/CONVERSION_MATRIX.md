# OdbcConvert Conversion Matrix

**Last Updated**: February 10, 2026  
**Source**: `src/OdbcConvert.cpp` — `getAdressFunction()` dispatch table

This document maps every (source SQL type, target C type) pair to the conversion function
that handles it in the Firebird ODBC driver.

## Legend

| Symbol | Meaning |
|--------|---------|
| ✅ | Implemented — conversion function exists and is dispatched |
| ❌ | Returns `notYetImplemented` (SQLSTATE 07006) |
| — | Not applicable (no dispatch path exists for this combination) |
| 🆔 | Identity conversion (marked `bIdentity = true` in dispatch) |

## Conversion Functions by Category

### Formatting Type

| Type | Description |
|------|-------------|
| **Identity** | Source and target are the same type; data is copied as-is |
| **Widening** | Lossless conversion to a larger type (e.g., SHORT→LONG) |
| **Narrowing** | Potentially lossy conversion to a smaller type (e.g., DOUBLE→SHORT) |
| **Formatting** | Type change requiring reformatting (e.g., INT→STRING, DATE→STRING) |
| **Parsing** | String→typed conversion requiring parsing (e.g., STRING→INT) |
| **Encoding** | Character encoding conversion (e.g., UTF-8→UTF-16) |

---

## Matrix: Numeric Source Types

### Source: SQL_TINYINT / SQL_C_TINYINT / SQL_C_UTINYINT

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_BIT | `convTinyIntToBit` | Narrowing | |
| SQL_C_TINYINT | `convTinyIntToTinyInt` | Identity | |
| SQL_C_SHORT | `convTinyIntToShort` | Widening | |
| SQL_C_LONG | `convTinyIntToLong` | Widening | |
| SQL_C_FLOAT | `convTinyIntToFloat` | Widening | |
| SQL_C_DOUBLE | `convTinyIntToDouble` | Widening | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convTinyIntToBigint` | Widening | |
| SQL_C_CHAR | `convTinyIntToString` | Formatting | 🆔 |
| SQL_C_WCHAR | `convTinyIntToStringW` | Formatting | |
| SQL_C_NUMERIC | `convLongToNumeric` | Formatting | |
| SQL_C_BINARY | ❌ | | ODBC spec says this should be supported |
| SQL_C_DATE/TIME/TIMESTAMP | ❌ | | Not valid per ODBC spec |

### Source: SQL_SMALLINT / SQL_C_SHORT / SQL_C_USHORT

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_BIT | `convShortToBit` | Narrowing | |
| SQL_C_TINYINT | `convShortToTinyInt` | Narrowing | |
| SQL_C_SHORT | `convShortToShort` | Identity | |
| SQL_C_LONG | `convShortToLong` | Widening | |
| SQL_C_FLOAT | `convShortToFloat` | Widening | |
| SQL_C_DOUBLE | `convShortToDouble` | Widening | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convShortToBigint` | Widening | |
| SQL_C_CHAR | `convShortToString` | Formatting | 🆔 |
| SQL_C_WCHAR | `convShortToStringW` | Formatting | |
| SQL_C_NUMERIC | `convLongToNumeric` | Formatting | |
| SQL_C_BINARY | ❌ | | ODBC spec says this should be supported |
| SQL_C_DATE/TIME/TIMESTAMP | ❌ | | Not valid per ODBC spec |

### Source: SQL_INTEGER / SQL_C_LONG / SQL_C_ULONG

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_BIT | `convLongToBit` | Narrowing | |
| SQL_C_TINYINT | `convLongToTinyInt` | Narrowing | |
| SQL_C_SHORT | `convLongToShort` | Narrowing | |
| SQL_C_LONG | `convLongToLong` | Identity | |
| SQL_C_FLOAT | `convLongToFloat` | Narrowing | |
| SQL_C_DOUBLE | `convLongToDouble` | Widening | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convLongToBigint` | Widening | |
| SQL_C_CHAR | `convLongToString` | Formatting | 🆔 |
| SQL_C_WCHAR | `convLongToStringW` | Formatting | |
| SQL_C_NUMERIC | `convLongToNumeric` | Formatting | |
| default | ❌ | | Catches BINARY, DATE/TIME/TIMESTAMP, GUID |

### Source: SQL_REAL / SQL_FLOAT

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_BIT | `convFloatToBit` | Narrowing | |
| SQL_C_TINYINT | `convFloatToTinyInt` | Narrowing | |
| SQL_C_SHORT | `convFloatToShort` | Narrowing | |
| SQL_C_LONG | `convFloatToLong` | Narrowing | |
| SQL_C_FLOAT | `convFloatToFloat` | Identity | |
| SQL_C_DOUBLE | `convFloatToDouble` | Widening | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convFloatToBigint` | Narrowing | |
| SQL_C_CHAR | `convFloatToString` | Formatting | 🆔 |
| SQL_C_WCHAR | `convFloatToStringW` | Formatting | |
| default | ❌ | | Missing: SQL_C_NUMERIC (spec says supported) |

### Source: SQL_DOUBLE / SQL_FLOAT (double precision)

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_BIT | `convDoubleToBit` | Narrowing | |
| SQL_C_TINYINT | `convDoubleToTinyInt` | Narrowing | |
| SQL_C_SHORT | `convDoubleToShort` | Narrowing | |
| SQL_C_LONG | `convDoubleToLong` | Narrowing | |
| SQL_C_FLOAT | `convDoubleToFloat` | Narrowing | |
| SQL_C_DOUBLE | `convDoubleToDouble` | Identity | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convDoubleToBigint` | Narrowing | |
| SQL_C_CHAR | `convDoubleToString` | Formatting | 🆔 |
| SQL_C_WCHAR | `convDoubleToStringW` | Formatting | |
| SQL_C_NUMERIC | `convDoubleToNumeric` | Formatting | |
| default | ❌ | | Missing: SQL_C_BINARY (spec says supported) |

### Source: SQL_BIGINT / SQL_C_SBIGINT / SQL_C_UBIGINT

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_TINYINT | `convBigintToTinyInt` | Narrowing | |
| SQL_C_SHORT | `convBigintToShort` | Narrowing | |
| SQL_C_LONG | `convBigintToLong` | Narrowing | |
| SQL_C_FLOAT | `convBigintToFloat` | Narrowing | |
| SQL_C_DOUBLE | `convBigintToDouble` | Widening | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convBigintToBigint` | Identity | |
| SQL_C_BINARY | `convBigintToBinary` | Identity | |
| SQL_C_CHAR | `convBigintToString` | Formatting | 🆔 |
| SQL_C_WCHAR | `convBigintToStringW` | Formatting | |
| SQL_C_NUMERIC | `convBigintToNumeric` | Formatting | |
| default | ❌ | | Missing: SQL_C_BIT (spec says supported) |

### Source: SQL_NUMERIC / SQL_DECIMAL

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_BIT | `convNumericToBit` | Narrowing | |
| SQL_C_TINYINT | `convNumericToTinyInt` | Narrowing | |
| SQL_C_SHORT | `convNumericToShort` | Narrowing | |
| SQL_C_LONG | `convNumericToLong` | Narrowing | |
| SQL_C_FLOAT | `convNumericToFloat` | Narrowing | |
| SQL_C_DOUBLE | `convNumericToDouble` | Widening | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convNumericToBigint` | Narrowing | |
| SQL_C_NUMERIC | `convNumericToNumeric` | Identity | |
| default | ❌ | | Missing: SQL_C_CHAR, SQL_C_WCHAR (spec says supported) |

---

## Matrix: Date/Time Source Types

### Source: SQL_DATE / SQL_TYPE_DATE

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_LONG | `convDateToLong` | Formatting | Julian day number |
| SQL_C_FLOAT | `convDateToFloat` | Formatting | |
| SQL_C_DOUBLE | `convDateToDouble` | Formatting | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convDateToBigint` | Formatting | |
| SQL_C_DATE / SQL_C_TYPE_DATE | `convDateToDate` | Identity | |
| SQL_C_TIMESTAMP / SQL_C_TYPE_TIMESTAMP | `convDateToTimestamp` | Widening | Time portion set to 00:00:00 |
| SQL_C_BINARY | `convDateToTagDate` | Identity | Raw ISC_DATE bytes |
| SQL_C_CHAR | `convDateToString` | Formatting | 🆔 |
| SQL_C_WCHAR | `convDateToStringW` | Formatting | |
| default | ❌ | | |

### Source: SQL_TIME / SQL_TYPE_TIME

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_LONG | `convTimeToLong` | Formatting | |
| SQL_C_FLOAT | `convTimeToFloat` | Formatting | |
| SQL_C_DOUBLE | `convTimeToDouble` | Formatting | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convTimeToBigint` | Formatting | |
| SQL_C_TIME / SQL_C_TYPE_TIME | `convTimeToTime` | Identity | |
| SQL_C_TIMESTAMP / SQL_C_TYPE_TIMESTAMP | `convTimeToTimestamp` | Widening | Date portion set to current date |
| SQL_C_BINARY | `convTimeToTagTime` | Identity | Raw ISC_TIME bytes |
| SQL_C_CHAR | `convTimeToString` | Formatting | 🆔 |
| SQL_C_WCHAR | `convTimeToStringW` | Formatting | |
| default | ❌ | | |

### Source: SQL_TIMESTAMP / SQL_TYPE_TIMESTAMP

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_DOUBLE | `convTimestampToDouble` | Formatting | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convTimestampToBigint` | Formatting | |
| SQL_C_DATE / SQL_C_TYPE_DATE | `convTimestampToDate` | Narrowing | Time portion discarded |
| SQL_C_TIME / SQL_C_TYPE_TIME | `convTimestampToTime` | Narrowing | Date portion discarded |
| SQL_C_TIMESTAMP / SQL_C_TYPE_TIMESTAMP | `convTimestampToTimestamp` | Identity | |
| SQL_C_BINARY | `convTimestampToTagTimestamp` | Identity | Raw ISC_TIMESTAMP bytes |
| SQL_C_CHAR | `convTimestampToString` | Formatting | 🆔 |
| SQL_C_WCHAR | `convTimestampToStringW` | Formatting | |
| default | ❌ | | |

---

## Matrix: String Source Types

### Source: SQL_CHAR (fixed-length, from VARCHAR wire format)

When `from->type` is VARCHAR/LONGVARCHAR, these are dispatched:

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_BIT | `convVarStringToBit` | Parsing | |
| SQL_C_TINYINT | `convVarStringToTinyInt` | Parsing | |
| SQL_C_SHORT | `convVarStringToShort` | Parsing | |
| SQL_C_LONG | `convVarStringToLong` | Parsing | |
| SQL_C_FLOAT | `convVarStringToFloat` | Parsing | |
| SQL_C_DOUBLE | `convVarStringToDouble` | Parsing | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convVarStringToBigint` | Parsing | |
| SQL_C_CHAR | `convVarStringToString` | Identity | 🆔 Trims trailing spaces for catalog results |
| SQL_C_WCHAR | `convVarStringToStringW` | Encoding | UTF-8 → UTF-16 via MbsToWcs; trims for catalog |
| SQL_C_BINARY | `convVarStringToBinary` | Identity | Raw bytes copied |
| SQL_C_DATE/TIME/TIMESTAMP | `transferStringToAllowedType` | Parsing | Only when `isIndicatorSqlDa` (server-side) |
| default | ❌ | | Missing: SQL_C_NUMERIC, SQL_C_GUID (spec says supported) |

When `from->type` is NOT VARCHAR (fixed-length CHAR):

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_CHAR | `convStringToString` | Identity | 🆔 |
| SQL_C_WCHAR | `convStringToStringW` | Encoding | UTF-8 → UTF-16 via MbsToWcs |
| SQL_C_BINARY | `convStringToBinary` | Identity | |
| SQL_C_DATE/TIME/TIMESTAMP | `transferStringToDateTime` | Parsing | Only when `isIndicatorSqlDa` |
| default | ❌ | | |

### Source: SQL_WCHAR (wide character, from WVARCHAR wire format)

When `from->type` is WVARCHAR/WLONGVARCHAR:

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_BIT | `convVarStringWToBit` | Parsing | |
| SQL_C_TINYINT | `convVarStringWToTinyInt` | Parsing | |
| SQL_C_SHORT | `convVarStringWToShort` | Parsing | |
| SQL_C_LONG | `convVarStringWToLong` | Parsing | |
| SQL_C_FLOAT | `convVarStringWToFloat` | Parsing | |
| SQL_C_DOUBLE | `convVarStringWToDouble` | Parsing | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convVarStringWToBigint` | Parsing | |
| SQL_C_CHAR | `convVarStringToString` | Encoding | Same as SQL_CHAR VARCHAR path |
| SQL_C_WCHAR | `convVarStringToStringW` | Identity | 🆔 Same as SQL_CHAR VARCHAR path |
| SQL_C_BINARY | `convVarStringToString` | Identity | Via SQL_C_BINARY→convVarStringToString |
| default | ❌ | | Missing: SQL_C_NUMERIC, SQL_C_GUID, DATE/TIME |

When `from->type` is NOT WVARCHAR (fixed-length WCHAR):

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_CHAR | `convStringToString` | Encoding | 🆔 |
| SQL_C_WCHAR | `convStringToStringW` | Identity | |
| SQL_C_BINARY | `convStringToBinary` | Identity | |
| SQL_C_GUID | `convStringToGuid` | Parsing | |
| SQL_C_DATE/TIME/TIMESTAMP | `transferStringToDateTime` | Parsing | Only when `isIndicatorSqlDa` |
| default | ❌ | | |

---

## Matrix: Binary / BLOB Source Types

### Source: SQL_BINARY / SQL_LONGVARBINARY

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_TINYINT | `convBinaryToTinyInt` | Identity | |
| SQL_C_SHORT | `convBinaryToShort` | Identity | |
| SQL_C_LONG | `convBinaryToLong` | Identity | |
| SQL_C_FLOAT | `convBinaryToFloat` | Identity | |
| SQL_C_DOUBLE | `convBinaryToDouble` | Identity | |
| SQL_C_SBIGINT / SQL_C_UBIGINT | `convBinaryToBigint` | Identity | |
| SQL_C_GUID | `convBinaryToGuid` | Identity | |
| SQL_C_BINARY | `convBinaryToBlob` or `convBinaryToBinary` | Identity | |
| SQL_C_CHAR | `convBlobToString` or `convBinaryToString` | Formatting | Hex encoding |
| SQL_C_WCHAR | `convBlobToStringW` or `convBinaryToStringW` | Formatting | |
| default | ❌ | | |

### Source: SQL_GUID

| Target C Type | Function | Type | Notes |
|---------------|----------|------|-------|
| SQL_C_CHAR | `convGuidToString` | Formatting | UUID string format |
| SQL_C_WCHAR | `convGuidToStringW` | Formatting | |
| SQL_C_BINARY | `convGuidToBinary` | Identity | |
| SQL_C_GUID | `convGuidToGuid` | Identity | |
| default | ❌ | | Correct per ODBC spec — GUID→numeric is not defined |

---

## Notes

### Catalog Result Set Trimming (Phase 12.3.1)

System catalog result sets (from `SQLTables`, `SQLColumns`, `SQLPrimaryKeys`, etc.) have
`isResultSetFromSystemCatalog = true`. The `convVarStringToString` and `convVarStringToStringW`
functions trim trailing spaces from VARCHAR data when this flag is set, because Firebird
sends catalog metadata as fixed-width CHAR (space-padded) inside VARCHAR wire format.

### Missing Conversions (Future Work)

The following ODBC-spec-required conversions are not yet implemented:

| Source → Target | ODBC Spec | Priority |
|----------------|-----------|----------|
| Integer types → SQL_C_BINARY | Required | Low |
| SQL_FLOAT → SQL_C_NUMERIC | Required | Medium |
| SQL_BIGINT → SQL_C_BIT | Required | Low |
| SQL_NUMERIC → SQL_C_CHAR/WCHAR | Required | **High** |
| String → SQL_C_NUMERIC | Required | Medium |
| String → SQL_C_DATE/TIME (app-side) | Required | Medium |
| String → SQL_C_GUID | Required | Low |

### Encoding Path (Phase 12.1)

All `*ToStringW` functions produce UTF-16 (SQLWCHAR) output using the column's charset codec:
- `from->MbsToWcs()` — dispatched per-column based on the connection's charset setting
- With `CHARSET=UTF8` (default since Phase 12.4.1), this is the UTF-8 → UTF-16 codec
- ASCII-only formatters (int→StringW, date→StringW, etc.) use trivial byte widening

All `*ToStringW` functions use `SQLWCHAR*` (not `wchar_t*`) for correct cross-platform behavior.
