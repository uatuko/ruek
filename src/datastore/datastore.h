#pragma once

#include "config.h"
#include "pg.h"

namespace datastore {
inline void init(const config &c = {}) {
	pg::init(c.pg);
}
} // namespace datastore
