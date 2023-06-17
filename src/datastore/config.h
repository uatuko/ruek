#pragma once

#include <string>

namespace datastore {
struct config {
	struct pg_t {
		std::string opts;
	};

	pg_t pg;
};
} // namespace datastore
