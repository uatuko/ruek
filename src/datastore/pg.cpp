#include "pg.h"

#include "err/errors.h"

static datastore::config::pg_t _conf;
static datastore::pg::conn_t   _conn = nullptr;

namespace datastore {
namespace pg {
conn_t::element_type &connection::conn() const {
	return *_conn;
}

conn_t::element_type &connection::reconnect() {
	_conn = connect();
	return *_conn;
}

connection conn() {
	if (!_conn) {
		throw err::DatastorePgConnectionUnavailable();
	}

	static std::timed_mutex mutex;
	if (!mutex.try_lock_for(_conf.timeout)) {
		throw err::DatastorePgTimeout();
	}

	return connection(_conn, connection::lock_t(mutex, std::adopt_lock));
}

conn_t connect() {
	// Ref: https://www.postgresql.org/docs/current/libpq-envars.html
	_conn = std::make_shared<conn_t::element_type>(_conf.opts);
	return _conn;
}

void init(const config::pg_t &c) {
	_conf = c;
	connect();
}
} // namespace pg
} // namespace datastore
