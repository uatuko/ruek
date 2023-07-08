#pragma once

#include <set>
#include <string>
#include <vector>

#include <glaze/glaze.hpp>

#include "pg.h"
#include "redis.h"

namespace datastore {
class AccessPolicy {
public:
	using identity_t  = std::string;
	using principal_t = std::string;
	using resource_t  = std::string;

	struct Rule {
		std::string attrs;
		std::string resource;

		bool operator<(const Rule &rhs) const noexcept { return resource < rhs.resource; }
	};

	struct Data {
		using name_t  = std::optional<std::string>;
		using rules_t = std::set<Rule>;

		std::string id;
		name_t      name;
		rules_t     rules;

		bool operator==(const Data &) const noexcept = default;
	};

	struct Record {
		identity_t identity_id;
		resource_t resource;

		const std::string key() const noexcept {
			return "access:(" + identity_id + ")›[" + resource + "]";
		};

		Record(const identity_t i, const resource_t r) : identity_id(i), resource(r) {}

		bool operator==(const Record &) const noexcept = default;

		void                      discard() const;
		std::vector<AccessPolicy> check() const;
	};

	AccessPolicy(const Data &data) noexcept;
	AccessPolicy(Data &&data) noexcept;

	AccessPolicy(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const Data::name_t &name() const noexcept { return _data.name; }
	void                name(const std::string &name) noexcept { _data.name = name; }
	void                name(std::string &&name) noexcept { _data.name = std::move(name); }

	void store() const;
	void discard() const;

	void add(const Record &record) const;
	void addIdentityPrincipal(const principal_t principalId) const;
	void addCollectionPrincipal(const principal_t principalId) const;

private:
	Data        _data;
	mutable int _rev;
};

using AccessPolicies = std::vector<AccessPolicy>;

AccessPolicy RetrieveAccessPolicy(const std::string &id);

std::set<std::string> RetrieveAccessPolicyIdentities(const std::string &id);
} // namespace datastore

template <> struct glz::meta<datastore::AccessPolicy::Rule> {
	using T                     = datastore::AccessPolicy::Rule;
	static constexpr auto value = object("attrs", &T::attrs, "resource", &T::resource);
};

namespace pqxx {
using rules_t = datastore::AccessPolicy::Data::rules_t;

template <> struct nullness<rules_t> {
	static constexpr bool has_null    = {true};
	static constexpr bool always_null = {false};

	static bool is_null(const rules_t &value) { return value.empty(); }

	[[nodiscard]] static rules_t null() { return {}; }
};

template <> struct string_traits<rules_t> {
	static rules_t from_string(std::string_view text) { return {}; }

	static char *into_buf(char *begin, char *end, rules_t const &value) {
		const auto buffer = glz::write_json(value);
		if (buffer.size() > (end - begin)) {
			throw pqxx::conversion_overrun("Not enough buffer capacity");
		}

		std::strcpy(begin, buffer.c_str());
		return (begin + buffer.size() + 1);
	}

	static std::size_t size_buffer(rules_t const &value) noexcept {
		std::size_t size = 2; // `[]`
		for (const auto &rule : value) {
			size += 3;                             // `{}` + `,`
			size += 8 + rule.attrs.size() + 2;     // `"attrs":"<value>",`
			size += rule.attrs.size() / 2;         // additional space for any escape chars
			size += 11 + rule.resource.size() + 2; // `"resource":"<value>"`
		}

		return size;
	}
};
} // namespace pqxx
