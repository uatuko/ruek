#include <thread>

#include <gtest/gtest.h>

#include "err/errors.h"

#include "redis.h"
#include "testing.h"

TEST(redis, cmd) {
	auto conf = datastore::testing::conf();
	try {
		datastore::redis::init(conf.redis);
	} catch (const std::exception &e) {
		FAIL() << e.what();
	}

	// Success - ping
	{
		auto conn  = datastore::redis::conn();
		auto reply = conn.cmd("PING");

		EXPECT_STREQ("PONG", reply->str);
	}
}

TEST(redis, concurrency) {
	auto conf          = datastore::testing::conf();
	conf.redis.timeout = 50ms;
	try {
		datastore::redis::init(conf.redis);
	} catch (const std::exception &e) {
		FAIL() << e.what();
	}

	// Success - timeout while waiting for connection lock
	{
		std::thread t1([conf]() {
			auto conn = datastore::redis::conn();
			std::this_thread::sleep_for(conf.redis.timeout * 3);
		});

		std::thread t2([]() {
			// Connection is locked into t1 scope, expect a timeout
			EXPECT_THROW(datastore::redis::conn(), err::DatastoreRedisTimeout);
		});

		t1.join();
		t2.join();
	}
}

TEST(redis, conn) {
	// Error - connection unavailable
	{ EXPECT_THROW(datastore::redis::conn(), err::DatastoreRedisConnectionUnavailable); }
}

TEST(redis, init) {
	// Error - connection failure
	{
		datastore::config::redis_t c = {
			.host = "invalid",
		};

		EXPECT_THROW(datastore::redis::init(c), err::DatastoreRedisConnectionFailure);
	}
}
