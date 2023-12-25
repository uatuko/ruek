#include <gtest/gtest.h>

#include "b32.h"

using namespace encoding;

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
