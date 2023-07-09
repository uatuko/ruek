#pragma once

#include <set>
#include <string>
#include <vector>

#include <glaze/glaze.hpp>

#include "pg.h"
#include "policies.h"

namespace datastore {
class AccessPolicy {
public:
	using collection_t  = std::string;
	using collections_t = std::set<collection_t>;
	using identity_t    = std::string;
	using identities_t  = std::set<identity_t>;

	struct Rule {
		const std::string attrs;
		const std::string resource;

		bool operator<(const Rule &rhs) const noexcept { return resource < rhs.resource; }
	};

	struct Cache {
		const std::string identity;
		const std::string policy;
		const Rule        rule;

		static const Policies check(const std::string &identity, const std::string &resource);

		static constexpr std::string key(const std::string &identity, const std::string &resource) {
			return "access:(" + identity + ")›[" + resource + "]";
		}

		constexpr std::string key() const noexcept { return key(identity, rule.resource); };

		void discard() const;
		void store() const;
	};

	struct Data {
		using name_t  = std::optional<std::string>;
		using rules_t = std::set<Rule>;

		std::string id;
		name_t      name;
		rules_t     rules;

		bool operator==(const Data &) const noexcept = default;
	};

	AccessPolicy(const Data &data) noexcept;
	AccessPolicy(Data &&data) noexcept;

	AccessPolicy(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const identities_t identities(bool expand = false) const;
	void               addIdentity(const identity_t &id) const;

	const Data::name_t &name() const noexcept { return _data.name; }
	void                name(const std::string &name) noexcept { _data.name = name; }
	void                name(std::string &&name) noexcept { _data.name = std::move(name); }

	const Data::rules_t &rules() const noexcept { return _data.rules; }

	void addCollection(const collection_t &id) const;

	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using AccessPolicies = std::vector<AccessPolicy>;

AccessPolicy RetrieveAccessPolicy(const std::string &id);
} // namespace datastore

template <> struct glz::meta<datastore::AccessPolicy::Rule> {
	using T                     = datastore::AccessPolicy::Rule;
	static constexpr auto value = object("attrs", &T::attrs, "resource", &T::resource);
};

namespace pqxx {
using access_rules_t = datastore::AccessPolicy::Data::rules_t;

template <> struct nullness<access_rules_t> {
	static constexpr bool has_null    = {true};
	static constexpr bool always_null = {false};

	static bool is_null(const access_rules_t &value) { return value.empty(); }

	[[nodiscard]] static access_rules_t null() { return {}; }
};

template <> struct string_traits<access_rules_t> {
	static access_rules_t from_string(std::string_view text) { return {}; }

	static char *into_buf(char *begin, char *end, access_rules_t const &value) {
		const auto buffer = glz::write_json(value);
		if (buffer.size() > (end - begin)) {
			throw pqxx::conversion_overrun("Not enough buffer capacity");
		}

		std::strcpy(begin, buffer.c_str());
		return (begin + buffer.size() + 1);
	}

	static std::size_t size_buffer(access_rules_t const &value) noexcept {
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
