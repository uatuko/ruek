#include <gtest/gtest.h>

#include "basic_error.h"

TEST(err_basic_error, str) {
	using E  = err::basic_error<"7000:0.0.0", "This is an error">;
	auto err = E();

	auto expected = std::string_view("[7000:0.0.0] This is an error");
	EXPECT_EQ(expected, err.str());
}
