#pragma once

#include <random>

namespace db {
namespace detail {
class rng {
public:
	rng();
	auto operator()() { return _mt19937(); }

private:
	std::mt19937 _mt19937;
};
} // namespace detail
} // namespace db
