#pragma once

#include <optional>
#include <string>
#include <vector>

#include "pg.h"

namespace db {
class Principal {
public:
	struct Data {
		using attrs_t   = std::optional<std::string>;
		using segment_t = std::optional<std::string>;

		attrs_t     attrs;
		std::string id;
		segment_t   segment;

		bool operator==(const Data &) const noexcept = default;
	};

	Principal(const Data &data) noexcept;
	Principal(Data &&data) noexcept;

	Principal(const pg::row_t &r);

	bool operator==(const Principal &) const noexcept = default;

	const Data::attrs_t &attrs() const noexcept { return _data.attrs; }
	void                 attrs(const Data::attrs_t &attrs) noexcept { _data.attrs = attrs; }
	void                 attrs(const std::string &attrs) noexcept { _data.attrs = attrs; }
	void                 attrs(Data::attrs_t &&attrs) noexcept { _data.attrs = std::move(attrs); }
	void                 attrs(std::string &&attrs) noexcept { _data.attrs = std::move(attrs); }

	const Data::segment_t &segment() const noexcept { return _data.segment; }
	void segment(const Data::segment_t &segment) noexcept { _data.segment = segment; }
	void segment(const std::string &segment) noexcept { _data.segment = segment; }
	void segment(Data::segment_t &&segment) noexcept { _data.segment = std::move(segment); }
	void segment(std::string &&segment) noexcept { _data.segment = std::move(segment); }

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	void store();

	static bool      discard(const std::string &id);
	static Principal retrieve(const std::string &id);

private:
	Data _data;
	int  _rev;
};

using Principals = std::vector<Principal>;

Principals ListPrincipals(
	Principal::Data::segment_t segment = std::nullopt, std::string_view lastId = "",
	uint16_t count = 10);
} // namespace db
