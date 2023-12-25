#pragma once

#include <string>
#include <string_view>

namespace encoding {
namespace b64 {
namespace {
static constexpr char alphabet[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

inline std::string encode(std::string_view in) {
	std::string out;
	out.reserve(4 * ((in.size() + 2) / 3)); // Encoding 3 bytes will result in 4 bytes

	int i = 0, j = -6;
	for (char c : in) {
		i  = (i << 8) + c;
		j += 8;

		while (j >= 0) {
			out.push_back(alphabet[(i >> j) & 0x3f]);
			j -= 6;
		}
	}

	if (j > -6) {
		out.push_back(alphabet[((i << 8) >> (j + 8)) & 0x3f]);
	}

	while (out.size() % 4) {
		out.push_back('=');
	}

	return out;
}
} // namespace b64
} // namespace encoding
