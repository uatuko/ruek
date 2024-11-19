#include "detail.h"

#include <random>

namespace db {
namespace detail {
int rand() {
	static std::mt19937 g;
	static bool         seeded = false;

	if (!seeded) {
		std::random_device rd;
		g.seed(rd());

		seeded = true;
	}

	return g();
}
} // namespace detail
} // namespace db
