#pragma once

namespace db {
inline void init(const config &c = {}) {
	pg::init(c);
}
} // namespace db
