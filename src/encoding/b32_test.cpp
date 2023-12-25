#include <gtest/gtest.h>

#include "b32.h"

using namespace encoding;

TEST(encoding_b32, decode) {
	{
		auto actual = b32::decode("91imor3f5gg7erridhi22");
		EXPECT_EQ("Hello, world!", actual);
	}

	// Test vectors from https://www.rfc-editor.org/rfc/rfc4648#section-10
	{
		EXPECT_EQ("", b32::decode(""));
		EXPECT_EQ("f", b32::decode("co"));
		EXPECT_EQ("fo", b32::decode("cpng"));
		EXPECT_EQ("foo", b32::decode("cpnmu"));
		EXPECT_EQ("foob", b32::decode("cpnmuog"));
		EXPECT_EQ("fooba", b32::decode("cpnmuoj1"));
		EXPECT_EQ("foobar", b32::decode("cpnmuoj1e8"));
	}

	// Error: invalid chars
	{
		EXPECT_EQ("", b32::decode("c+o"));
		EXPECT_EQ("f", b32::decode("cpn+g"));
	}
}

TEST(encoding_b32, encode) {
	{
		auto actual = b32::encode("Hello, world!");
		EXPECT_EQ("91imor3f5gg7erridhi22", actual);
	}

	// Test vectors from https://www.rfc-editor.org/rfc/rfc4648#section-10
	{
		EXPECT_EQ("", b32::encode(""));
		EXPECT_EQ("co", b32::encode("f"));
		EXPECT_EQ("cpng", b32::encode("fo"));
		EXPECT_EQ("cpnmu", b32::encode("foo"));
		EXPECT_EQ("cpnmuog", b32::encode("foob"));
		EXPECT_EQ("cpnmuoj1", b32::encode("fooba"));
		EXPECT_EQ("cpnmuoj1e8", b32::encode("foobar"));
	}
}
