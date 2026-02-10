/*
 *  UTF-16 Conversion Utilities for ODBC Unicode Support
 *  
 *  This file provides platform-independent UTF-8 ↔ UTF-16 conversion
 *  for proper ODBC Unicode API support (Issue #244).
 *  
 *  CRITICAL: SQLWCHAR is ALWAYS 16-bit UTF-16 (UCS-2) per ODBC spec,
 *  regardless of platform wchar_t size.
 *  
 *  Note: Include this header AFTER including sql.h/sqlext.h to get SQLWCHAR definition
 */

#ifndef _UTF16_CONVERT_H_
#define _UTF16_CONVERT_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Convert UTF-8 to UTF-16 (for ODBC Unicode APIs)
// Returns: number of SQLWCHAR units written (excluding null terminator)
// If utf16 is NULL, returns required buffer size
size_t Utf8ToUtf16(const char* utf8, SQLWCHAR* utf16, size_t utf16BufferSize);

// Convert UTF-16 to UTF-8 (for Firebird)
// Returns: number of bytes written (excluding null terminator)
// If utf8 is NULL, returns required buffer size
size_t Utf16ToUtf8(const SQLWCHAR* utf16, char* utf8, size_t utf8BufferSize);

// Get length of UTF-16 string in SQLWCHAR units (like wcslen but for SQLWCHAR)
size_t Utf16Length(const SQLWCHAR* str);

// Get number of SQLWCHAR units needed to encode UTF-8 string
size_t Utf8ToUtf16Length(const char* utf8);

// Get number of UTF-8 bytes needed to encode UTF-16 string
size_t Utf16ToUtf8Length(const SQLWCHAR* utf16);

// Count UTF-16 code units in a potentially partial string
// Returns number of complete characters (code points), accounting for surrogates
size_t Utf16CountChars(const SQLWCHAR* str, size_t utf16Units);

// Platform-independent UTF-16 string copy
SQLWCHAR* Utf16Copy(SQLWCHAR* dest, const SQLWCHAR* src, size_t maxUnits);

// Platform-independent UTF-16 string comparison
int Utf16Compare(const SQLWCHAR* s1, const SQLWCHAR* s2);

#ifdef __cplusplus
}
#endif

#endif // _UTF16_CONVERT_H_
