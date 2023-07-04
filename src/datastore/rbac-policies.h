#pragma once

#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class RbacPolicy {
public:
	struct Data {
		using rules_t = std::optional<std::string>;

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

  const Data::rules_t &rules() const noexcept { return _data.rules; }
	void                 rules(const Data::rules_t &rules) noexcept { _data.rules = rules; }
	void                 rules(const std::string &rules) noexcept { _data.rules = rules; }
	void                 rules(Data::rules_t &&rules) noexcept { _data.rules = std::move(rules); }
	void                 rules(std::string &&rules) noexcept { _data.rules = std::move(rules); }

	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using RbacPolicies = std::vector<RbacPolicy>;
} // namespace datastore
