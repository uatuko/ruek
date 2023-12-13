#include "pg.h"

#include <optional>

#include "err/errors.h"

static db::config                    _conf;
static std::optional<db::pg::conn_t> _conn = std::nullopt;

namespace db {
namespace pg {
connection conn() {
	if (!_conn) {
		throw err::DbConnectionUnavailable();
	}

	static std::timed_mutex mutex;
	if (!mutex.try_lock_for(_conf.timeout)) {
		throw err::DbTimeout();
	}

	return connection(_conn.value(), connection::lock_t(mutex, std::adopt_lock));
}

conn_t &connect() {
	// Ref: https://www.postgresql.org/docs/current/libpq-envars.html
	_conn = conn_t(_conf.opts);
	return _conn.value();
}

void init(const config &c) {
	_conf = c;
	connect();
}
} // namespace pg
} // namespace db
