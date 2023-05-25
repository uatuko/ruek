#pragma once

#include <algorithm>
#include <string_view>

namespace err {
template <std::size_t N> struct fixed_string {
	constexpr fixed_string(const char (&str)[N]) { std::copy_n(str, N, value); }

	inline const char *c_str() const noexcept { return value; }

	constexpr inline std::size_t size() const noexcept { return N; }

	inline std::string_view str() const noexcept { return value; }

	char value[N];
};
} // namespace err
