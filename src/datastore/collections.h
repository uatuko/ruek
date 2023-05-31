#pragma once

#include <set>
#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class Collection {
public:
	using member_t  = std::string;
	using members_t = std::set<const member_t>;

	struct Data {
		std::string id;
		std::string name;

		bool operator==(const Data &) const noexcept = default;
	};

	Collection(const Data &data) noexcept;
	Collection(Data &&data) noexcept;

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &name() const noexcept { return _data.name; }

	const members_t members() const;
	void            add(const member_t &id) const;
	void            remove(const member_t &id) const;

	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using Collections = std::vector<Collection>;
} // namespace datastore
