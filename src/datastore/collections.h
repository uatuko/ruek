#pragma once

#include <set>
#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class Collection {
public:
	using member_t  = std::string;
	using members_t = std::set<member_t>;

	struct Data {
		std::string id;
		std::string name;

		bool operator==(const Data &) const noexcept = default;
	};

	Collection(const Data &data) noexcept;
	Collection(Data &&data) noexcept;

	Collection(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &name() const noexcept { return _data.name; }
	void               name(const std::string &name) noexcept { _data.name = name; }
	void               name(std::string &&name) noexcept { _data.name = std::move(name); }

	const members_t members() const;
	void            add(const member_t &id) const;
	void            remove(const member_t &id) const;

	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using Collections = std::vector<Collection>;

Collection RetrieveCollection(const std::string &id);
} // namespace datastore
