#pragma once

#include "config.h"
#include "pg.h"
#include "redis.h"

namespace datastore {
inline void init(const config &c = {}) {
	pg::init(c.pg);
	redis::init(c.redis);
}
} // namespace datastore
