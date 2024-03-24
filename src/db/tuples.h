#pragma once

#include <optional>
#include <string>

#include "pg.h"

namespace db {
class Tuple {
public:
	using rid_t = std::optional<std::string>;

	struct Data {
		using attrs_t = std::optional<std::string>;
		using pid_t   = std::optional<std::string>;

		attrs_t attrs;

		std::string lEntityId;
		std::string lEntityType;
		pid_t       lPrincipalId;

		std::string relation;

		std::string rEntityId;
		std::string rEntityType;
		pid_t       rPrincipalId;

		std::string spaceId;
		std::string strand;

		bool operator==(const Data &) const noexcept = default;
	};

	Tuple(const Data &data) noexcept;
	Tuple(Data &&data) noexcept;

	Tuple(const pg::row_t &r);

	bool operator==(const Tuple &) const noexcept = default;

	const Data::attrs_t &attrs() const noexcept { return _data.attrs; }

	const std::string &lEntityId() const noexcept { return _data.lEntityId; }
	const std::string &lEntityType() const noexcept { return _data.lEntityType; }
	const Data::pid_t &lPrincipalId() const noexcept { return _data.lPrincipalId; }

	const std::string &relation() const noexcept { return _data.relation; }

	const std::string &rEntityId() const noexcept { return _data.rEntityId; }
	const std::string &rEntityType() const noexcept { return _data.rEntityType; }
	const Data::pid_t &rPrincipalId() const noexcept { return _data.rPrincipalId; }

	const std::string &spaceId() const noexcept { return _data.spaceId; }
	const std::string &strand() const noexcept { return _data.strand; }

	const std::string &id() const noexcept { return _id; }
	const int         &rev() const noexcept { return _rev; }
	const rid_t       &rid() const noexcept { return _rid; }

	void store();

	static Tuple retrieve(const std::string &id);

	static std::optional<Tuple> lookup(
		std::string_view spaceId, std::string_view strand, std::string_view lEntityType,
		std::string_view lEntityId, std::string_view relation, std::string_view rEntityType,
		std::string_view rEntityId);

private:
	void sanitise() noexcept;

	Data        _data;
	std::string _id;
	int         _rev;
	rid_t       _rid;
};
} // namespace db
