#pragma once

#include <cstring>
#include <stdexcept>
#include <string_view>

#include "fixed_string.h"

namespace err {
template <fixed_string C, fixed_string M> struct basic_error : public std::runtime_error {
public:
	basic_error() : std::runtime_error(M.c_str()) {
		std::strcat(_err, "[");
		std::strcat(_err, C.c_str());
		std::strcat(_err, "] ");
		std::strcat(_err, M.c_str());
	}

	inline std::string_view str() const noexcept { return _err; }

	friend std::ostream &operator<<(std::ostream &os, const basic_error &err) {
		return os << err.str();
	}

protected:
	char _err[C.size() + M.size() + 2] = {};
};
} // namespace err
