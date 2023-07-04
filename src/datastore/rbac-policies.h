#pragma once

#include <set>
#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class RbacPolicy {
public:
	using rules_t = std::set<std::string>;
	using member_t  = std::string;
	using members_t = std::set<member_t>;
	using role_t  = std::string;
	using roles_t = std::set<role_t>;
	struct Data {
		std::string id;
    std::string name;
    rules_t     rules;

		bool operator==(const Data &) const noexcept = default;
	};

	RbacPolicy(const Data &data) noexcept;
	RbacPolicy(Data &&data) noexcept;

	RbacPolicy(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &name() const noexcept { return _data.name; }
	void               name(const std::string &name) noexcept { _data.name = name; }
	void               name(std::string &&name) noexcept { _data.name = std::move(name); }

  const rules_t &rules() const noexcept { return _data.rules; }
	void                 rules(const rules_t &rules) noexcept { _data.rules = rules; }
	void                 rules(rules_t &&rules) noexcept { _data.rules = std::move(rules); }

	const members_t members() const;
	void            addMember(const member_t &id) const;
	void            removeMember(const member_t &id) const;

	const roles_t roles() const;
	void          addRole(const role_t &id) const;
	void          removeRole(const role_t &id) const;

	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using RbacPolicies = std::vector<RbacPolicy>;
} // namespace datastore

namespace pqxx {
using T = datastore::RbacPolicy::rules_t;

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