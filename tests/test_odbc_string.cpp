// Phase 12 (12.2.1): Unit tests for OdbcString — UTF-16-native string class.

#include <gtest/gtest.h>
#include "OdbcString.h"

using namespace OdbcJdbcLibrary;

class OdbcStringTest : public ::testing::Test {};

TEST_F(OdbcStringTest, DefaultConstructorIsEmpty)
{
    OdbcString s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(0, s.length());
    EXPECT_EQ(0, s.byte_length());
    EXPECT_NE(nullptr, s.data());  // data() should never return nullptr
    EXPECT_EQ((SQLWCHAR)0, s.data()[0]);
}

TEST_F(OdbcStringTest, FromAscii)
{
    OdbcString s = OdbcString::from_ascii("HELLO");
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(5, s.length());
    EXPECT_EQ(5 * (int)sizeof(SQLWCHAR), s.byte_length());
    EXPECT_EQ((SQLWCHAR)'H', s.data()[0]);
    EXPECT_EQ((SQLWCHAR)'E', s.data()[1]);
    EXPECT_EQ((SQLWCHAR)'L', s.data()[2]);
    EXPECT_EQ((SQLWCHAR)'L', s.data()[3]);
    EXPECT_EQ((SQLWCHAR)'O', s.data()[4]);
    EXPECT_EQ((SQLWCHAR)0,   s.data()[5]);
}

TEST_F(OdbcStringTest, FromAsciiWithLength)
{
    OdbcString s = OdbcString::from_ascii("HELLO WORLD", 5);
    EXPECT_EQ(5, s.length());
    EXPECT_EQ((SQLWCHAR)'H', s.data()[0]);
    EXPECT_EQ((SQLWCHAR)'O', s.data()[4]);
}

TEST_F(OdbcStringTest, FromAsciiNull)
{
    OdbcString s = OdbcString::from_ascii(nullptr);
    EXPECT_TRUE(s.empty());
}

TEST_F(OdbcStringTest, FromAsciiEmpty)
{
    OdbcString s = OdbcString::from_ascii("");
    EXPECT_TRUE(s.empty());
}

TEST_F(OdbcStringTest, FromUtf8SimpleAscii)
{
    OdbcString s = OdbcString::from_utf8("Test123");
    EXPECT_EQ(7, s.length());
    EXPECT_EQ((SQLWCHAR)'T', s.data()[0]);
    EXPECT_EQ((SQLWCHAR)'3', s.data()[6]);
}

TEST_F(OdbcStringTest, FromUtf8MultiByte)
{
    // UTF-8: "ü" = 0xC3 0xBC (U+00FC)
    OdbcString s = OdbcString::from_utf8("\xC3\xBC");
    EXPECT_EQ(1, s.length());
    EXPECT_EQ((SQLWCHAR)0x00FC, s.data()[0]);
}

TEST_F(OdbcStringTest, FromUtf8ThreeByte)
{
    // UTF-8: "€" = 0xE2 0x82 0xAC (U+20AC)
    OdbcString s = OdbcString::from_utf8("\xE2\x82\xAC");
    EXPECT_EQ(1, s.length());
    EXPECT_EQ((SQLWCHAR)0x20AC, s.data()[0]);
}

TEST_F(OdbcStringTest, FromUtf16)
{
    SQLWCHAR utf16[] = { 'A', 'B', 'C', 0 };
    OdbcString s = OdbcString::from_utf16(utf16);
    EXPECT_EQ(3, s.length());
    EXPECT_EQ((SQLWCHAR)'A', s.data()[0]);
    EXPECT_EQ((SQLWCHAR)'C', s.data()[2]);
}

TEST_F(OdbcStringTest, FromUtf16WithLength)
{
    SQLWCHAR utf16[] = { 'A', 'B', 'C', 'D', 0 };
    OdbcString s = OdbcString::from_utf16(utf16, 2);
    EXPECT_EQ(2, s.length());
    EXPECT_EQ((SQLWCHAR)'A', s.data()[0]);
    EXPECT_EQ((SQLWCHAR)'B', s.data()[1]);
}

TEST_F(OdbcStringTest, ToUtf8Ascii)
{
    OdbcString s = OdbcString::from_ascii("Hello");
    std::string utf8 = s.to_utf8();
    EXPECT_EQ("Hello", utf8);
}

TEST_F(OdbcStringTest, ToUtf8MultiByte)
{
    // U+00FC (ü)
    SQLWCHAR utf16[] = { 0x00FC, 0 };
    OdbcString s = OdbcString::from_utf16(utf16);
    std::string utf8 = s.to_utf8();
    EXPECT_EQ("\xC3\xBC", utf8);
}

TEST_F(OdbcStringTest, ToUtf8Empty)
{
    OdbcString s;
    std::string utf8 = s.to_utf8();
    EXPECT_TRUE(utf8.empty());
}

TEST_F(OdbcStringTest, CopyConstructor)
{
    OdbcString orig = OdbcString::from_ascii("Copy");
    OdbcString copy(orig);
    EXPECT_EQ(4, copy.length());
    EXPECT_EQ((SQLWCHAR)'C', copy.data()[0]);
    // Ensure deep copy — modifying orig shouldn't affect copy
    EXPECT_NE(orig.data(), copy.data());
}

TEST_F(OdbcStringTest, CopyAssignment)
{
    OdbcString a = OdbcString::from_ascii("Alpha");
    OdbcString b = OdbcString::from_ascii("Bravo");
    b = a;
    EXPECT_EQ(5, b.length());
    EXPECT_EQ((SQLWCHAR)'A', b.data()[0]);
}

TEST_F(OdbcStringTest, MoveConstructor)
{
    OdbcString orig = OdbcString::from_ascii("Move");
    SQLWCHAR* origPtr = orig.data();
    OdbcString moved(std::move(orig));
    EXPECT_EQ(4, moved.length());
    EXPECT_EQ(origPtr, moved.data());  // should take ownership
    EXPECT_TRUE(orig.empty());  // NOLINT: we test moved-from state
}

TEST_F(OdbcStringTest, MoveAssignment)
{
    OdbcString a = OdbcString::from_ascii("First");
    OdbcString b = OdbcString::from_ascii("Second");
    b = std::move(a);
    EXPECT_EQ(5, b.length());
    EXPECT_EQ((SQLWCHAR)'F', b.data()[0]);
}

TEST_F(OdbcStringTest, Equality)
{
    OdbcString a = OdbcString::from_ascii("Same");
    OdbcString b = OdbcString::from_ascii("Same");
    OdbcString c = OdbcString::from_ascii("Diff");
    OdbcString d;

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);

    OdbcString e;
    EXPECT_EQ(d, e);
}

TEST_F(OdbcStringTest, Clear)
{
    OdbcString s = OdbcString::from_ascii("ClearMe");
    EXPECT_FALSE(s.empty());
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(0, s.length());
}

TEST_F(OdbcStringTest, CopyToWBufferFull)
{
    OdbcString s = OdbcString::from_ascii("Test");  // 4 chars
    SQLWCHAR buf[10];
    bool truncated = false;
    SQLLEN total = s.copy_to_w_buffer(buf, sizeof(buf), &truncated);

    EXPECT_EQ(4 * (SQLLEN)sizeof(SQLWCHAR), total);
    EXPECT_FALSE(truncated);
    EXPECT_EQ((SQLWCHAR)'T', buf[0]);
    EXPECT_EQ((SQLWCHAR)'t', buf[3]);
    EXPECT_EQ((SQLWCHAR)0,   buf[4]);
}

TEST_F(OdbcStringTest, CopyToWBufferTruncated)
{
    OdbcString s = OdbcString::from_ascii("LongString");  // 10 chars
    SQLWCHAR buf[5];  // room for 4 chars + null (buf size = 5 * sizeof(SQLWCHAR) = 10 bytes)
    bool truncated = false;
    SQLLEN total = s.copy_to_w_buffer(buf, sizeof(buf), &truncated);

    EXPECT_EQ(10 * (SQLLEN)sizeof(SQLWCHAR), total);  // reports full length
    EXPECT_TRUE(truncated);
    EXPECT_EQ((SQLWCHAR)'L', buf[0]);
    EXPECT_EQ((SQLWCHAR)'g', buf[3]);
    EXPECT_EQ((SQLWCHAR)0,   buf[4]);
}

TEST_F(OdbcStringTest, CopyToWBufferNullBuffer)
{
    OdbcString s = OdbcString::from_ascii("Test");
    SQLLEN total = s.copy_to_w_buffer(nullptr, 0);
    EXPECT_EQ(4 * (SQLLEN)sizeof(SQLWCHAR), total);
}

TEST_F(OdbcStringTest, CopyToABufferFull)
{
    OdbcString s = OdbcString::from_ascii("Test");
    char buf[10];
    bool truncated = false;
    SQLLEN total = s.copy_to_a_buffer(buf, sizeof(buf), &truncated);

    EXPECT_EQ(4, total);
    EXPECT_FALSE(truncated);
    EXPECT_STREQ("Test", buf);
}

TEST_F(OdbcStringTest, CopyToABufferTruncated)
{
    OdbcString s = OdbcString::from_ascii("LongString");
    char buf[5];  // room for 4 chars + null
    bool truncated = false;
    SQLLEN total = s.copy_to_a_buffer(buf, sizeof(buf), &truncated);

    EXPECT_EQ(10, total);  // reports full length
    EXPECT_TRUE(truncated);
    EXPECT_STREQ("Long", buf);
}

TEST_F(OdbcStringTest, RoundTripUtf8)
{
    // Round-trip: UTF-8 → OdbcString → UTF-8
    const char* original = "Hello, world! \xC3\xBC\xE2\x82\xAC";  // "Hello, world! ü€"
    OdbcString s = OdbcString::from_utf8(original);
    std::string result = s.to_utf8();
    EXPECT_EQ(std::string(original), result);
}

TEST_F(OdbcStringTest, RoundTripUtf16)
{
    // Round-trip: UTF-16 → OdbcString → UTF-16
    SQLWCHAR original[] = { 'H', 'i', 0x00FC, 0x20AC, 0 };
    OdbcString s = OdbcString::from_utf16(original);
    EXPECT_EQ(4, s.length());
    EXPECT_EQ(original[0], s.data()[0]);
    EXPECT_EQ(original[1], s.data()[1]);
    EXPECT_EQ(original[2], s.data()[2]);
    EXPECT_EQ(original[3], s.data()[3]);
}
