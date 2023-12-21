#pragma once

#include <optional>
#include <string>
#include <vector>

#include "pg.h"

namespace db {
class Principal {
public:
	struct Data {
		using attrs_t = std::optional<std::string>;
		using pid_t   = std::optional<std::string>;

		attrs_t     attrs;
		std::string id;
		pid_t       parentId;
	};

	Principal(const Data &data) noexcept;
	Principal(Data &&data) noexcept;

	Principal(const pg::row_t &t);

	const Data::attrs_t &attrs() const noexcept { return _data.attrs; }
	void                 attrs(const Data::attrs_t &attrs) noexcept { _data.attrs = attrs; }
	void                 attrs(const std::string &attrs) noexcept { _data.attrs = attrs; }
	void                 attrs(Data::attrs_t &&attrs) noexcept { _data.attrs = std::move(attrs); }
	void                 attrs(std::string &&attrs) noexcept { _data.attrs = std::move(attrs); }

	const Data::pid_t &parentId() const noexcept { return _data.parentId; }
	void               parentId(const Data::pid_t &parentId) noexcept { _data.parentId = parentId; }
	void               parentId(const std::string &parentId) noexcept { _data.parentId = parentId; }
	void parentId(Data::pid_t &&parentId) noexcept { _data.parentId = std::move(parentId); }
	void parentId(std::string &&parentId) noexcept { _data.parentId = std::move(parentId); }

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	void discard();
	void store();

private:
	Data _data;
	int  _rev;
};

using Principals = std::vector<Principal>;

Principal RetrievePrincipal(const std::string &id);
} // namespace db
