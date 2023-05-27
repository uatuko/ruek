#pragma once

#include <cstdlib>

#include "datastore.h"

namespace datastore {
namespace testing {
void setup() {
	const char *v = std::getenv("PGDATABASE");
	init("dbname=test-" + std::string(v == nullptr ? "" : v));
}

void teardown() {}
} // namespace testing
} // namespace datastore
