#pragma once

#include <cstdlib>

#include <viper/viper.h>

#include "datastore.h"

namespace datastore {
namespace testing {
inline config conf() {
	const char *v = std::getenv("PGDATABASE");

	auto dbname = std::string(v == nullptr ? "" : v);
	if (!dbname.starts_with("test-")) {
		dbname = "test-" + dbname;
	}

	viper::init("app", GATEKEEPER_TEST_CONF_PATH);
	auto conf = *viper::conf();

	return {
		.pg =
			{
				.opts = "dbname=" + dbname,
			},
		.redis =
			{
				.host = conf["redis.host"].get<std::string>(),
				.port = static_cast<int>(conf["redis.port"].get<long>()),
			},
	};
}

inline void setup() {
	init(conf());
}

inline void teardown() {}
} // namespace testing
} // namespace datastore
