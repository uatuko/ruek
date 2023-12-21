#pragma once

#include <optional>
#include <string>
#include <vector>

#include "pg.h"

namespace db {
class Record {
public:
	struct Data {
		using attrs_t = std::optional<std::string>;

		attrs_t     attrs;
		std::string principalId;
		std::string resourceId;
		std::string resourceType;
	};

	Record(const Data &data) noexcept;
	Record(Data &&data) noexcept;

	const Data::attrs_t &attrs() const noexcept { return _data.attrs; }
	void                 attrs(const Data::attrs_t &attrs) noexcept { _data.attrs = attrs; }
	void                 attrs(const std::string &attrs) noexcept { _data.attrs = attrs; }
	void                 attrs(Data::attrs_t &&attrs) noexcept { _data.attrs = std::move(attrs); }
	void                 attrs(std::string &&attrs) noexcept { _data.attrs = std::move(attrs); }

	const std::string &principalId() const noexcept { return _data.principalId; }
	const std::string &resourceId() const noexcept { return _data.resourceId; }
	const std::string &resourceType() const noexcept { return _data.resourceType; }

	const int &rev() const noexcept { return _rev; }

	void discard();
	void store();

private:
	Data _data;
	int  _rev;
};

using Records = std::vector<Record>;
} // namespace db
