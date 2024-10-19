#include "detail.h"

#include <random>

namespace db {
namespace detail {
int rng() {
	static std::random_device rd;
	static std::mt19937       g(rd());

	return g();
}
} // namespace detail
} // namespace db
