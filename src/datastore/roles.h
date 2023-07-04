#pragma once

#include <set>
#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class Role {
public:
	using permissions_t = std::set<std::string>;

	struct Data {
		std::string   id;
		std::string   name;
		permissions_t permissions;

		bool operator==(const Data &) const noexcept = default;
	};

	Role(const Data &data) noexcept;
	Role(Data &&data) noexcept;

	Role(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &name() const noexcept { return _data.name; }
	void               name(const std::string &name) noexcept { _data.name = name; }
	void               name(std::string &&name) noexcept { _data.name = std::move(name); }

	const permissions_t &permissions() const noexcept { return _data.permissions; }
	void permissions(const permissions_t &permissions) noexcept { _data.permissions = permissions; }
	void permissions(permissions_t &&permissions) noexcept {
		_data.permissions = std::move(permissions);
	}

	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using Roles = std::vector<Role>;

Role RetrieveRole(const std::string &id);
} // namespace datastore

namespace pqxx {
using T = datastore::Role::permissions_t;

template <> struct nullness<T> {
	static constexpr bool has_null = {true};

	[[nodiscard]] static T null() { return {}; }
};

template <> struct string_traits<T> {
	static T from_string(std::string_view text) {
		// FIXME: this is a very optimistic lookup, expects `{string,string}` with no escaped chars
		T result;

		using size_t = std::string_view::size_type;
		for (size_t next, pos = 1; pos < text.size() - 1; pos = next + 1) {
			next = text.find(',', pos);
			if (next == text.npos) {
				result.insert(std::string(text.substr(pos, (text.size() - 1) - pos)));
				break;
			}

			result.insert(std::string(text.substr(pos, next - pos)));
		}

		return result;
	}
};
} // namespace pqxx
