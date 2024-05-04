#pragma once

#include <optional>
#include <string>
#include <vector>

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

	class Entity {
	public:
		Entity() = default;
		Entity(std::string_view pid) noexcept;
		Entity(std::string_view type, std::string_view id) noexcept;

		std::string_view id() const noexcept { return _id; }
		std::string_view type() const noexcept { return _type; }

	private:
		std::string_view _id;
		std::string_view _type;
	};

	Tuple(const Data &data) noexcept;
	Tuple(Data &&data) noexcept;

	Tuple(const pg::row_t &r);

	bool operator==(const Tuple &) const noexcept = default;

	const Data::attrs_t &attrs() const noexcept { return _data.attrs; }
	void                 attrs(std::string &&attrs) noexcept { _data.attrs = std::move(attrs); }

	const std::string &lEntityId() const noexcept { return _data.lEntityId; }
	const std::string &lEntityType() const noexcept { return _data.lEntityType; }
	const Data::pid_t &lPrincipalId() const noexcept { return _data.lPrincipalId; }

	void lPrincipalId(const std::string &pid) noexcept {
		_data.lPrincipalId = pid;
		sanitise();
	}

	const std::string &relation() const noexcept { return _data.relation; }

	const std::string &rEntityId() const noexcept { return _data.rEntityId; }
	const std::string &rEntityType() const noexcept { return _data.rEntityType; }
	const Data::pid_t &rPrincipalId() const noexcept { return _data.rPrincipalId; }

	void rPrincipalId(const std::string &pid) noexcept {
		_data.rPrincipalId = pid;
		sanitise();
	}

	const std::string &spaceId() const noexcept { return _data.spaceId; }
	const std::string &strand() const noexcept { return _data.strand; }

	const std::string &id() const noexcept { return _id; }
	const int         &rev() const noexcept { return _rev; }
	const rid_t       &ridL() const noexcept { return _ridL; }
	const rid_t       &ridR() const noexcept { return _ridR; }

	void store();

	static bool discard(std::string_view id);

	static std::optional<Tuple> lookup(
		std::string_view spaceId, Entity left, Entity right, std::string_view relation = "",
		std::string_view strand = "");

	static Tuple retrieve(std::string_view id);

private:
	void sanitise() noexcept;

	Data        _data;
	std::string _id;
	int         _rev;
	rid_t       _ridL;
	rid_t       _ridR;
};

using Tuples = std::vector<Tuple>;

Tuples ListTuples(
	std::string_view spaceId, std::optional<Tuple::Entity> left, std::optional<Tuple::Entity> right,
	std::optional<std::string_view> relation = std::nullopt, std::string_view lastId = "",
	std::uint16_t count = 10);

Tuples ListTuplesLeft(
	std::string_view spaceId, Tuple::Entity right, std::optional<std::string_view> relation,
	std::string_view lastId = "", std::uint16_t count = 10);

Tuples ListTuplesRight(
	std::string_view spaceId, Tuple::Entity left, std::optional<std::string_view> relation,
	std::string_view lastId = "", std::uint16_t count = 10);

Tuples LookupTuples(
	std::string_view spaceId, Tuple::Entity left, std::string_view relation, Tuple::Entity right,
	std::optional<std::string_view> strand = std::nullopt, std::string_view lastId = "",
	std::uint16_t count = 10);
} // namespace db
