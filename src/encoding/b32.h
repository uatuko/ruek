#pragma once

#include <string>
#include <string_view>

namespace encoding {
namespace b32 {
namespace {
static constexpr char alphabet[] = "0123456789abcdefghijklmnopqrstuv";
}

inline std::string encode(std::string_view in) {
	std::string out;
	out.reserve(
		((in.size() * 8) + 4) / 5); // Encoding 5 bytes will result in 8 bytes (less padding)

	int i = 0, j = -5;
	for (char c : in) {
		i  = (i << 8) + c;
		j += 8;

		while (j >= 0) {
			out.push_back(alphabet[(i >> j) & 0x1f]);
			j -= 5;
		}
	}

	if (j > -5) {
		out.push_back(alphabet[((i << 8) >> (j + 8)) & 0x1f]);
	}

	return out;
}
}; // namespace b32
} // namespace encoding
