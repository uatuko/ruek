#pragma once

#include <string>
#include <vector>

namespace datastore {
struct Policy {
	const std::string attrs;
	const std::string id;
};

using Policies = std::vector<Policy>;
} // namespace datastore
