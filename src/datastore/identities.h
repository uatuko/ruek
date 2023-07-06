#pragma once

#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class Identity {
public:
	struct Data {
		using attrs_t = std::optional<std::string>;

		attrs_t     attrs;
		std::string id;
		std::string sub;

		bool operator==(const Data &) const noexcept = default;
	};

	Identity(const Data &data) noexcept;
	Identity(Data &&data) noexcept;

	Identity(const pg::row_t &t);

	const Data::attrs_t &attrs() const noexcept { return _data.attrs; }
	void                 attrs(const Data::attrs_t &attrs) noexcept { _data.attrs = attrs; }
	void                 attrs(const std::string &attrs) noexcept { _data.attrs = attrs; }
	void                 attrs(Data::attrs_t &&attrs) noexcept { _data.attrs = std::move(attrs); }
	void                 attrs(std::string &&attrs) noexcept { _data.attrs = std::move(attrs); }

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &sub() const noexcept { return _data.sub; }
	void               sub(const std::string &sub) noexcept { _data.sub = sub; }
	void               sub(std::string &&sub) noexcept { _data.sub = std::move(sub); }

	void discard() const;
	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using Identities = std::vector<Identity>;

std::vector<std::string> ListIdentitiesInCollection(const std::string &id);
Identity                 RetrieveIdentity(const std::string &id);
} // namespace datastore
