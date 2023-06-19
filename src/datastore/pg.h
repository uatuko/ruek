#pragma once

#include <pqxx/pqxx>

#include "config.h"

namespace datastore {
namespace pg {
using conn_t   = pqxx::connection;
using row_t    = pqxx::row;
using result_t = pqxx::result;
using nontxn_t = pqxx::nontransaction;

using fkey_violation_t   = pqxx::foreign_key_violation;
using unique_violation_t = pqxx::unique_violation;

std::shared_ptr<conn_t> conn();

result_t exec(std::string_view qry);

template <typename... Args> inline result_t exec(std::string_view qry, Args &&...args) {
	nontxn_t tx(*conn());
	return tx.exec_params(pqxx::zview(qry), args...);
}

void init(const config::pg_t &c);
} // namespace pg
} // namespace datastore
