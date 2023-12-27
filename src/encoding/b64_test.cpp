#include <gtest/gtest.h>

#include "b64.h"

using namespace encoding;

TEST(encoding_b64, encode) {
	{
		auto actual = b64::encode("Hello, world!");
		EXPECT_EQ("SGVsbG8sIHdvcmxkIQ==", actual);
	}

	// Test vectors from https://www.rfc-editor.org/rfc/rfc4648#section-10
	{
		EXPECT_EQ("", b64::encode(""));
		EXPECT_EQ("Zg==", b64::encode("f"));
		EXPECT_EQ("Zm8=", b64::encode("fo"));
		EXPECT_EQ("Zm9v", b64::encode("foo"));
		EXPECT_EQ("Zm9vYg==", b64::encode("foob"));
		EXPECT_EQ("Zm9vYmE=", b64::encode("fooba"));
		EXPECT_EQ("Zm9vYmFy", b64::encode("foobar"));
	}
}
