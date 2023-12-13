#pragma once

#include <mutex>

#include <pqxx/pqxx>

#include "config.h"

namespace db {
namespace pg {
using conn_t   = pqxx::connection;
using row_t    = pqxx::row;
using result_t = pqxx::result;
using nontxn_t = pqxx::nontransaction;

using fkey_violation_t   = pqxx::foreign_key_violation;
using unique_violation_t = pqxx::unique_violation;

conn_t &connect();

class connection {
public:
	using lock_t = std::unique_lock<std::timed_mutex>;

	connection(conn_t &conn, lock_t &&lock) noexcept : _conn(conn), _lock(std::move(lock)) {}

	auto exec(std::string_view qry, auto &&...args) {
		try {
			return nontxn_exec(qry, std::forward<decltype(args)>(args)...);
		} catch (const pqxx::broken_connection &) {
			// Try to reconnect, if it fails will throw an error
			connect();
		}

		return nontxn_exec(qry, std::forward<decltype(args)>(args)...);
	}

private:
	result_t nontxn_exec(std::string_view qry, auto &&...args) const {
		nontxn_t tx(_conn);
		return tx.exec_params(pqxx::zview(qry), std::forward<decltype(args)>(args)...);
	}

	conn_t &_conn;
	lock_t  _lock;
};

connection conn();

inline auto exec(std::string_view qry, auto &&...args) {
	return conn().exec(qry, std::forward<decltype(args)>(args)...);
}

void init(const config &c);
} // namespace pg
} // namespace db
