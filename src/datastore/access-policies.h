#pragma once

#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class AccessPolicy {
public:
	struct Data {
		using rules_t = std::optional<std::string>;

		rules_t     rules;
		std::string id;
		std::string name;

		bool operator==(const Data &) const noexcept = default;
	};

	AccessPolicy(const Data &data) noexcept;
	AccessPolicy(Data &&data) noexcept;

	AccessPolicy(const pg::row_t &t);

	const Data::rules_t  &rules() const noexcept { return _data.rules; }
	void                 rules(const Data::rules_t &rules) noexcept { _data.rules = rules; }
	void                 rules(const std::string &rules) noexcept { _data.rules = rules; }
	void                 rules(Data::rules_t &&rules) noexcept { _data.rules = std::move(rules); }
	void                 rules(std::string &&rules) noexcept { _data.rules = std::move(rules); }

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &name() const noexcept { return _data.name; }
	void               name(const std::string &name) noexcept { _data.name = name; }
	void               name(std::string &&name) noexcept { _data.name = std::move(name); }

	void store()   const;
	void discard() const;

private:
	Data        _data;
	mutable int _rev;
};

using AccessPolicies = std::vector<AccessPolicy>;

AccessPolicy RetrieveAccessPolicy(const std::string &id);
} // namespace datastore
