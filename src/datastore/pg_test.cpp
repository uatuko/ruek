#include <thread>

#include <gtest/gtest.h>

#include "pg.h"
#include "testing.h"

TEST(pg, concurrency) {
	if (std::thread::hardware_concurrency() < 2) {
		GTEST_SKIP() << "Not enough hardware support to run concurrency tests";
	}

	auto conf       = datastore::testing::conf();
	conf.pg.timeout = 50ms;
	ASSERT_NO_THROW(datastore::pg::init(conf.pg));

	// Success: timeout while waiting for connection lock
	{
		std::thread t1([conf]() {
			auto conn = datastore::pg::conn();
			std::this_thread::sleep_for(conf.pg.timeout * 5);
		});

		std::thread t2([]() {
			// Connection is locked into t1 scope, expect a timeout
			EXPECT_THROW(datastore::pg::conn(), err::DatastorePgTimeout);
		});

		t1.join();
		t2.join();
	}
}

TEST(pg, conn) {
	// Error: connection unavailable
	{ EXPECT_THROW(datastore::pg::conn(), err::DatastorePgConnectionUnavailable); }
}

TEST(pg, reconnect) {
	auto conf = datastore::testing::conf();
	ASSERT_NO_THROW(datastore::pg::init(conf.pg));

	// Success: reconnect
	{
		auto c = datastore::pg::connect();
		c->close();

		EXPECT_NO_THROW(datastore::pg::exec("select 'ping';"));
	}
}
