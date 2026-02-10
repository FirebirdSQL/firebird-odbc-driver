#pragma once

// Phase 12 (12.2.1): UTF-16-native string class for internal metadata storage.
//
// OdbcString is a thin wrapper around a dynamically-allocated SQLWCHAR array
// that provides:
//   - UTF-16 storage compatible with SQLWCHAR* (always 16-bit on all platforms)
//   - Conversion to/from UTF-8 via Utf16Convert.h
//   - Safe copy to ODBC application buffers with truncation handling
//   - Value semantics (copyable, movable)
//
// This type gradually replaces JString and char* for metadata storage
// throughout the ODBC layer (OdbcError, DescRecord, OdbcStatement).

#include <cstring>
#include <string>

#include "OdbcJdbc.h"
#include "Utf16Convert.h"

namespace OdbcJdbcLibrary {

class OdbcString {
public:
	// Default: empty string
	OdbcString() noexcept : data_(nullptr), length_(0) {}

	// Destructor
	~OdbcString() { delete[] data_; }

	// Copy
	OdbcString(const OdbcString& other)
		: data_(nullptr), length_(other.length_)
	{
		if (length_ > 0) {
			data_ = new SQLWCHAR[length_ + 1];
			memcpy(data_, other.data_, (length_ + 1) * sizeof(SQLWCHAR));
		}
	}

	OdbcString& operator=(const OdbcString& other)
	{
		if (this != &other) {
			OdbcString tmp(other);
			swap(tmp);
		}
		return *this;
	}

	// Move
	OdbcString(OdbcString&& other) noexcept
		: data_(other.data_), length_(other.length_)
	{
		other.data_ = nullptr;
		other.length_ = 0;
	}

	OdbcString& operator=(OdbcString&& other) noexcept
	{
		if (this != &other) {
			delete[] data_;
			data_ = other.data_;
			length_ = other.length_;
			other.data_ = nullptr;
			other.length_ = 0;
		}
		return *this;
	}

	// --- Factory methods ---

	// Create from UTF-8 data. If len < 0, reads until null terminator.
	static OdbcString from_utf8(const char* utf8, int len = -1)
	{
		OdbcString s;
		if (!utf8)
			return s;

		if (len < 0)
			len = (int)strlen(utf8);

		if (len == 0)
			return s;

		// Worst case: each UTF-8 byte could produce one SQLWCHAR
		// (actually max is len, since the shortest UTF-8 sequence is 1 byte per codepoint)
		s.data_ = new SQLWCHAR[len + 1];
		int converted = (int)Utf8ToUtf16(utf8, s.data_, len + 1);
		if (converted < 0)
			converted = 0;
		s.length_ = converted;
		s.data_[s.length_] = (SQLWCHAR)0;
		return s;
	}

	// Create from UTF-16 data. If len < 0, reads until null terminator.
	static OdbcString from_utf16(const SQLWCHAR* utf16, int len = -1)
	{
		OdbcString s;
		if (!utf16)
			return s;

		if (len < 0)
			len = (int)Utf16Length(utf16);

		if (len == 0)
			return s;

		s.data_ = new SQLWCHAR[len + 1];
		memcpy(s.data_, utf16, len * sizeof(SQLWCHAR));
		s.data_[len] = (SQLWCHAR)0;
		s.length_ = len;
		return s;
	}

	// Create from an ASCII C string (fast path: direct byte→SQLWCHAR widening).
	// Only safe for pure ASCII data (0x00–0x7F). Used for SQLSTATE codes, fixed labels.
	static OdbcString from_ascii(const char* ascii, int len = -1)
	{
		OdbcString s;
		if (!ascii)
			return s;

		if (len < 0)
			len = (int)strlen(ascii);

		if (len == 0)
			return s;

		s.data_ = new SQLWCHAR[len + 1];
		for (int i = 0; i < len; ++i)
			s.data_[i] = (SQLWCHAR)(unsigned char)ascii[i];
		s.data_[len] = (SQLWCHAR)0;
		s.length_ = len;
		return s;
	}

	// --- Accessors ---

	// Pointer to the internal UTF-16 data. Returns empty string (not nullptr) if empty.
	SQLWCHAR* data() noexcept { return data_ ? data_ : const_cast<SQLWCHAR*>(kEmpty); }
	const SQLWCHAR* data() const noexcept { return data_ ? data_ : kEmpty; }

	// Length in SQLWCHAR units (not including null terminator).
	SQLSMALLINT length() const noexcept { return (SQLSMALLINT)length_; }

	// Length in bytes (SQLWCHAR units * sizeof(SQLWCHAR)).
	SQLLEN byte_length() const noexcept { return (SQLLEN)(length_ * sizeof(SQLWCHAR)); }

	// Whether the string is empty.
	bool empty() const noexcept { return length_ == 0; }

	// --- Conversion ---

	// Convert to UTF-8 std::string.
	std::string to_utf8() const
	{
		if (length_ == 0)
			return std::string();

		// Worst case: each SQLWCHAR can produce up to 3 UTF-8 bytes (BMP)
		// or 4 bytes for surrogate pairs
		int maxBytes = length_ * 4 + 1;
		std::string result(maxBytes, '\0');
		int converted = (int)Utf16ToUtf8(data_, &result[0], maxBytes);
		if (converted < 0)
			converted = 0;
		result.resize(converted);
		return result;
	}

	// --- ODBC buffer operations ---

	// Copy to an application's SQLWCHAR* buffer with proper truncation handling.
	// bufferLength is in bytes (as per ODBC convention for W functions).
	// Returns the total string length in bytes (regardless of truncation).
	// Sets *truncated to true if the data was truncated.
	SQLLEN copy_to_w_buffer(SQLWCHAR* buffer, SQLLEN bufferLength, bool* truncated = nullptr) const
	{
		SQLLEN totalBytes = byte_length();

		if (truncated)
			*truncated = false;

		if (!buffer || bufferLength <= 0)
			return totalBytes;

		// Available space in SQLWCHAR units (minus 1 for null terminator)
		int maxChars = (int)(bufferLength / sizeof(SQLWCHAR)) - 1;
		if (maxChars < 0)
			maxChars = 0;

		int copyChars = (length_ < maxChars) ? length_ : maxChars;
		if (copyChars > 0)
			memcpy(buffer, data(), copyChars * sizeof(SQLWCHAR));
		buffer[copyChars] = (SQLWCHAR)0;

		if (copyChars < length_ && truncated)
			*truncated = true;

		return totalBytes;
	}

	// Copy to an application's char* buffer (converts UTF-16 → UTF-8).
	// bufferLength is in bytes.
	// Returns the total UTF-8 string length in bytes (regardless of truncation).
	SQLLEN copy_to_a_buffer(char* buffer, SQLLEN bufferLength, bool* truncated = nullptr) const
	{
		std::string utf8 = to_utf8();
		SQLLEN totalBytes = (SQLLEN)utf8.size();

		if (truncated)
			*truncated = false;

		if (!buffer || bufferLength <= 0)
			return totalBytes;

		int maxBytes = (int)bufferLength - 1;  // room for null terminator
		if (maxBytes < 0)
			maxBytes = 0;

		int copyBytes = ((int)utf8.size() < maxBytes) ? (int)utf8.size() : maxBytes;
		if (copyBytes > 0)
			memcpy(buffer, utf8.c_str(), copyBytes);
		buffer[copyBytes] = '\0';

		if (copyBytes < (int)utf8.size() && truncated)
			*truncated = true;

		return totalBytes;
	}

	// --- Comparison ---

	bool operator==(const OdbcString& other) const noexcept
	{
		if (length_ != other.length_)
			return false;
		if (length_ == 0)
			return true;
		return memcmp(data_, other.data_, length_ * sizeof(SQLWCHAR)) == 0;
	}

	bool operator!=(const OdbcString& other) const noexcept
	{
		return !(*this == other);
	}

	// --- Utility ---

	void clear() noexcept
	{
		delete[] data_;
		data_ = nullptr;
		length_ = 0;
	}

	void swap(OdbcString& other) noexcept
	{
		SQLWCHAR* tmpData = data_;
		int tmpLen = length_;
		data_ = other.data_;
		length_ = other.length_;
		other.data_ = tmpData;
		other.length_ = tmpLen;
	}

private:
	SQLWCHAR* data_;
	int length_;  // in SQLWCHAR units, not including null terminator

	static inline const SQLWCHAR kEmpty[1] = { 0 };
};

} // namespace OdbcJdbcLibrary
