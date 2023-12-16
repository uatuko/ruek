#pragma once

#include "config.h"
#include "pg.h"

namespace db {
inline void init(const config &c = {}) {
	pg::init(c);
}
} // namespace db
