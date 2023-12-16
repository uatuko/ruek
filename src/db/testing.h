#pragma once

#include "config.h"
#include "db.h"

namespace db {
namespace testing {
inline config conf() {
	const char *v = std::getenv("PGDATABASE");

	auto dbname = std::string(v == nullptr ? "" : v);
	if (!dbname.starts_with("test-")) {
		dbname = "test-" + dbname;
	}

	return {
		.opts = "dbname=" + dbname,
	};
}

inline void setup() {
	init(conf());
}

inline void teardown() {}
} // namespace testing
} // namespace db
