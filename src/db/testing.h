#pragma once

#include "config.h"

namespace db {
namespace testing {
config conf() {
	const char *v = std::getenv("PGDATABASE");

	auto dbname = std::string(v == nullptr ? "" : v);
	if (!dbname.starts_with("test-")) {
		dbname = "test-" + dbname;
	}

	return {
		.opts = "dbname=" + dbname,
	};
}
} // namespace testing
} // namespace db
