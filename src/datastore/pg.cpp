#include "pg.h"

static std::shared_ptr<datastore::pg::conn_t> _conn = nullptr;

namespace datastore {
namespace pg {
std::shared_ptr<conn_t> conn() {
	// FIXME: check if initialised
	return _conn;
}

result_t exec(std::string_view qry) {
	nontxn_t tx(*conn());
	return tx.exec(qry);
}

void init(const config::pg_t &c) {
	// Ref: https://www.postgresql.org/docs/current/libpq-envars.html
	_conn = std::make_shared<conn_t>(c.opts);
}
} // namespace pg
} // namespace datastore
