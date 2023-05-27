#pragma once

#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class Identity {
public:
	struct Data {
		std::string id;
		std::string sub;

		bool operator==(const Data &) const noexcept = default;
	};

	Identity(const Data &data) noexcept;
	Identity(Data &&data) noexcept;

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &sub() const noexcept { return _data.sub; }

	void discard() const;
	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using Identities = std::vector<Identity>;

Identity RetrieveIdentity(const std::string &id);
} // namespace datastore
