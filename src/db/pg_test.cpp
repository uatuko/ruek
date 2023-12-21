#include <thread>

#include <gtest/gtest.h>

#include "err/errors.h"

#include "pg.h"
#include "testing.h"

TEST(db_pg, concurrency) {
	if (std::thread::hardware_concurrency() < 3) {
		GTEST_SKIP() << "Not enough hardware support to run concurrency tests";
	}

	auto conf    = db::testing::conf();
	conf.timeout = 50ms;
	ASSERT_NO_THROW(db::pg::init(conf));

	// Success: timeout while waiting for connection lock
	{
		std::thread t1([conf]() {
			auto conn = db::pg::conn();
			std::this_thread::sleep_for(conf.timeout * 5);
		});

		std::thread t2([]() {
			// Connection is locked into t1 scope, expect a timeout
			EXPECT_THROW(db::pg::conn(), err::DbTimeout);
		});

		t1.join();
		t2.join();
	}
}

TEST(db_pg, conn) {
	// Error: connection unavailable
	{ EXPECT_THROW(db::pg::conn(), err::DbConnectionUnavailable); }
}

TEST(db_pg, reconnect) {
	auto conf = db::testing::conf();
	ASSERT_NO_THROW(db::pg::init(conf));

	// Success: reconnect
	{
		auto &c = db::pg::connect();
		c.close();

		EXPECT_NO_THROW(db::pg::exec("select 'ping';"));
	}
}
