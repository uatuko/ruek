#pragma once

#include <pqxx/pqxx>

#include "config.h"

namespace datastore {
namespace pg {
using conn_t   = std::shared_ptr<pqxx::connection>;
using row_t    = pqxx::row;
using result_t = pqxx::result;
using nontxn_t = pqxx::nontransaction;

using fkey_violation_t   = pqxx::foreign_key_violation;
using unique_violation_t = pqxx::unique_violation;

class connection {
public:
	using lock_t = std::unique_lock<std::timed_mutex>;

	connection(const conn_t &conn, lock_t &&lock) noexcept : _conn(conn), _lock(std::move(lock)) {}

	auto exec(std::string_view qry, auto &&...args) {
		try {
			return nontxn_exec(qry, std::forward<decltype(args)>(args)...);
		} catch (const pqxx::broken_connection &e) {
			// Try to reconnect, if it fails will throw an error
			reconnect();
		}

		return nontxn_exec(qry, std::forward<decltype(args)>(args)...);
	}

private:
	result_t nontxn_exec(std::string_view qry, auto &&...args) const {
		nontxn_t tx(conn());
		return tx.exec_params(pqxx::zview(qry), std::forward<decltype(args)>(args)...);
	}

	conn_t::element_type &conn() const;
	conn_t::element_type &reconnect();

	conn_t _conn;
	lock_t _lock;
};

connection conn();
conn_t     connect();

inline auto exec(std::string_view qry, auto &&...args) {
	return conn().exec(qry, std::forward<decltype(args)>(args)...);
}

void init(const config::pg_t &c);
} // namespace pg
} // namespace datastore
