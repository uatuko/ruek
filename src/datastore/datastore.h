#pragma once

#include "pg.h"

namespace datastore {
inline auto init() {
	return pg::init();
}

inline auto init(const std::string &opts) {
	return pg::init(opts);
}
} // namespace datastore
