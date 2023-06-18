#include "redis.h"

static datastore::config::redis_t  _conf;
static datastore::redis::context_t _context = nullptr;
static std::timed_mutex            _mutex;

namespace datastore {
namespace redis {
context_t::element_type *connection::ctx() const {
	return _ctx.get();
}

connection conn() {
	if (!_context) {
		throw err::DatastoreRedisConnectionUnavailable();
	}

	if (!_mutex.try_lock_for(_conf.timeout)) {
		throw err::DatastoreRedisTimeout();
	}

	return connection(_context, connection::lock_t(_mutex, std::adopt_lock));
}

void init(const config::redis_t &c) {
	_conf = c;

	timeval timeout(c.timeout);

	redisOptions opts = {
		.connect_timeout = &timeout,
		.command_timeout = &timeout,
	};
	REDIS_OPTIONS_SET_TCP(&opts, c.host.c_str(), c.port);

	_context = context_t(redisConnectWithOptions(&opts), redisFree);

	if (_context == nullptr || _context->err) {
		throw err::DatastoreRedisConnectionFailure();
	}
}
} // namespace redis
} // namespace datastore
