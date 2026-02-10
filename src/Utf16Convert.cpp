/*
 *  UTF-16 Conversion Utilities for ODBC Unicode Support
 *  
 *  This file provides platform-independent UTF-8 ↔ UTF-16 conversion
 *  for proper ODBC Unicode API support (Issue #244).
 */

#ifdef _WINDOWS
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include "Utf16Convert.h"
#include <string.h>

// Helper macros for UTF-16 surrogate pairs (guarded — windows.h may define these)
#ifndef IS_HIGH_SURROGATE
#define IS_HIGH_SURROGATE(wc) ((wc) >= 0xD800 && (wc) <= 0xDBFF)
#endif
#ifndef IS_LOW_SURROGATE
#define IS_LOW_SURROGATE(wc) ((wc) >= 0xDC00 && (wc) <= 0xDFFF)
#endif
#ifndef IS_SURROGATE
#define IS_SURROGATE(wc) ((wc) >= 0xD800 && (wc) <= 0xDFFF)
#endif

// Calculate code point from surrogate pair
#define SURROGATE_TO_CODEPOINT(hi, lo) \
    (((((hi) - 0xD800) << 10) | ((lo) - 0xDC00)) + 0x10000)

// Calculate surrogate pair from code point
#define CODEPOINT_TO_HIGH_SURROGATE(cp) ((SQLWCHAR)(((cp) - 0x10000) >> 10) + 0xD800)
#define CODEPOINT_TO_LOW_SURROGATE(cp) ((SQLWCHAR)(((cp) - 0x10000) & 0x3FF) + 0xDC00)

size_t Utf16Length(const SQLWCHAR* str)
{
    if (!str)
        return 0;
    
    size_t len = 0;
    while (str[len] != 0)
        len++;
    
    return len;
}

size_t Utf8ToUtf16(const char* utf8, SQLWCHAR* utf16, size_t utf16BufferSize)
{
    if (!utf8)
        return 0;
    
    const unsigned char* src = (const unsigned char*)utf8;
    size_t utf16Pos = 0;
    
    // If utf16 is NULL, we're just calculating required size
    bool calculateOnly = (utf16 == NULL);
    
    while (*src)
    {
        unsigned int codepoint;
        size_t utf8Bytes;
        
        // Determine UTF-8 sequence length and decode
        if ((*src & 0x80) == 0)
        {
            // 1-byte sequence (ASCII)
            codepoint = *src;
            utf8Bytes = 1;
        }
        else if ((*src & 0xE0) == 0xC0)
        {
            // 2-byte sequence
            if ((src[1] & 0xC0) != 0x80)
                break; // Invalid UTF-8
            codepoint = ((src[0] & 0x1F) << 6) | (src[1] & 0x3F);
            utf8Bytes = 2;
        }
        else if ((*src & 0xF0) == 0xE0)
        {
            // 3-byte sequence
            if ((src[1] & 0xC0) != 0x80 || (src[2] & 0xC0) != 0x80)
                break; // Invalid UTF-8
            codepoint = ((src[0] & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F);
            utf8Bytes = 3;
        }
        else if ((*src & 0xF8) == 0xF0)
        {
            // 4-byte sequence
            if ((src[1] & 0xC0) != 0x80 || (src[2] & 0xC0) != 0x80 || (src[3] & 0xC0) != 0x80)
                break; // Invalid UTF-8
            codepoint = ((src[0] & 0x07) << 18) | ((src[1] & 0x3F) << 12) | 
                       ((src[2] & 0x3F) << 6) | (src[3] & 0x3F);
            utf8Bytes = 4;
        }
        else
        {
            // Invalid UTF-8 start byte
            break;
        }
        
        // Encode as UTF-16
        if (codepoint < 0x10000)
        {
            // BMP character - single UTF-16 unit
            if (!calculateOnly)
            {
                if (utf16Pos >= utf16BufferSize - 1) // Reserve space for null terminator
                    break;
                utf16[utf16Pos] = (SQLWCHAR)codepoint;
            }
            utf16Pos++;
        }
        else if (codepoint <= 0x10FFFF)
        {
            // Supplementary character - surrogate pair
            if (!calculateOnly)
            {
                if (utf16Pos >= utf16BufferSize - 2) // Need 2 units + null terminator
                    break;
                utf16[utf16Pos] = CODEPOINT_TO_HIGH_SURROGATE(codepoint);
                utf16[utf16Pos + 1] = CODEPOINT_TO_LOW_SURROGATE(codepoint);
            }
            utf16Pos += 2;
        }
        else
        {
            // Invalid code point
            break;
        }
        
        src += utf8Bytes;
    }
    
    if (!calculateOnly && utf16Pos < utf16BufferSize)
        utf16[utf16Pos] = 0; // Null terminate
    
    return utf16Pos;
}

size_t Utf16ToUtf8(const SQLWCHAR* utf16, char* utf8, size_t utf8BufferSize)
{
    if (!utf16)
        return 0;
    
    size_t utf8Pos = 0;
    size_t utf16Pos = 0;
    
    // If utf8 is NULL, we're just calculating required size
    bool calculateOnly = (utf8 == NULL);
    
    while (utf16[utf16Pos] != 0)
    {
        unsigned int codepoint;
        
        // Decode UTF-16
        SQLWCHAR unit = utf16[utf16Pos++];
        
        if (IS_HIGH_SURROGATE(unit))
        {
            // Surrogate pair
            if (utf16[utf16Pos] == 0 || !IS_LOW_SURROGATE(utf16[utf16Pos]))
            {
                // Invalid surrogate pair
                break;
            }
            codepoint = SURROGATE_TO_CODEPOINT(unit, utf16[utf16Pos]);
            utf16Pos++;
        }
        else if (IS_LOW_SURROGATE(unit))
        {
            // Invalid - low surrogate without high surrogate
            break;
        }
        else
        {
            // BMP character
            codepoint = unit;
        }
        
        // Encode as UTF-8
        if (codepoint < 0x80)
        {
            // 1-byte UTF-8
            if (!calculateOnly)
            {
                if (utf8Pos >= utf8BufferSize - 1) // Reserve space for null terminator
                    break;
                utf8[utf8Pos] = (char)codepoint;
            }
            utf8Pos++;
        }
        else if (codepoint < 0x800)
        {
            // 2-byte UTF-8
            if (!calculateOnly)
            {
                if (utf8Pos >= utf8BufferSize - 2)
                    break;
                utf8[utf8Pos] = (char)(0xC0 | (codepoint >> 6));
                utf8[utf8Pos + 1] = (char)(0x80 | (codepoint & 0x3F));
            }
            utf8Pos += 2;
        }
        else if (codepoint < 0x10000)
        {
            // 3-byte UTF-8
            if (!calculateOnly)
            {
                if (utf8Pos >= utf8BufferSize - 3)
                    break;
                utf8[utf8Pos] = (char)(0xE0 | (codepoint >> 12));
                utf8[utf8Pos + 1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                utf8[utf8Pos + 2] = (char)(0x80 | (codepoint & 0x3F));
            }
            utf8Pos += 3;
        }
        else if (codepoint <= 0x10FFFF)
        {
            // 4-byte UTF-8
            if (!calculateOnly)
            {
                if (utf8Pos >= utf8BufferSize - 4)
                    break;
                utf8[utf8Pos] = (char)(0xF0 | (codepoint >> 18));
                utf8[utf8Pos + 1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
                utf8[utf8Pos + 2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                utf8[utf8Pos + 3] = (char)(0x80 | (codepoint & 0x3F));
            }
            utf8Pos += 4;
        }
    }
    
    if (!calculateOnly && utf8Pos < utf8BufferSize)
        utf8[utf8Pos] = '\0'; // Null terminate
    
    return utf8Pos;
}

size_t Utf8ToUtf16Length(const char* utf8)
{
    return Utf8ToUtf16(utf8, NULL, 0);
}

size_t Utf16ToUtf8Length(const SQLWCHAR* utf16)
{
    return Utf16ToUtf8(utf16, NULL, 0);
}

size_t Utf16CountChars(const SQLWCHAR* str, size_t utf16Units)
{
    if (!str)
        return 0;
    
    size_t chars = 0;
    size_t pos = 0;
    
    while (pos < utf16Units && str[pos] != 0)
    {
        if (IS_HIGH_SURROGATE(str[pos]))
        {
            // Surrogate pair counts as one character
            if (pos + 1 < utf16Units && IS_LOW_SURROGATE(str[pos + 1]))
            {
                pos += 2;
                chars++;
            }
            else
            {
                // Invalid surrogate pair
                break;
            }
        }
        else if (IS_LOW_SURROGATE(str[pos]))
        {
            // Invalid - unpaired low surrogate
            break;
        }
        else
        {
            // BMP character
            pos++;
            chars++;
        }
    }
    
    return chars;
}

SQLWCHAR* Utf16Copy(SQLWCHAR* dest, const SQLWCHAR* src, size_t maxUnits)
{
    if (!dest || !src || maxUnits == 0)
        return dest;
    
    size_t i = 0;
    while (i < maxUnits - 1 && src[i] != 0)
    {
        dest[i] = src[i];
        i++;
    }
    
    dest[i] = 0; // Null terminate
    return dest;
}

int Utf16Compare(const SQLWCHAR* s1, const SQLWCHAR* s2)
{
    if (!s1 || !s2)
        return s1 == s2 ? 0 : (s1 ? 1 : -1);
    
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    
    return (int)((unsigned int)*s1 - (unsigned int)*s2);
}
