#include "rng.h"

static std::random_device rd;

namespace db {
namespace detail {
rng::rng() : _mt19937(rd()) {}
} // namespace detail
} // namespace db
