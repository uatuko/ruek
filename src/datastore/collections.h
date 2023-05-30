#pragma once

#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class Collection {
public:
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

	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using Collections = std::vector<Collection>;
} // namespace datastore
