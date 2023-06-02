#pragma once

#include <cstdlib>

#include "datastore.h"

namespace datastore {
namespace testing {
static void setup() {
	const char *v = std::getenv("PGDATABASE");

	auto dbname = std::string(v == nullptr ? "" : v);
	if (!dbname.starts_with("test-")) {
		dbname = "test-" + dbname;
	}

	init("dbname=" + dbname);
}

static void teardown() {}
} // namespace testing
} // namespace datastore
