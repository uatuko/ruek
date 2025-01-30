#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common.h"
#include "pg.h"

namespace db {
class Tuple {
public:
	using pid_t = std::optional<std::string>;
	using rid_t = std::optional<std::string>;

	struct Data {
		using attrs_t = std::optional<std::string>;

		attrs_t attrs;

		std::string lEntityId;
		std::string lEntityType;

		std::string relation;

		std::string rEntityId;
		std::string rEntityType;

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

		std::int64_t hash() const noexcept;

	private:
		std::string_view _id;
		std::string_view _type;
	};

	Tuple(const Data &data) noexcept;
	Tuple(Data &&data) noexcept;

	Tuple(const pg::row_t &r);

	Tuple(const Tuple &left, const Tuple &right) noexcept;

	bool operator==(const Tuple &) const noexcept = default;

	const Data::attrs_t &attrs() const noexcept { return _data.attrs; }
	void                 attrs(std::string &&attrs) noexcept { _data.attrs = std::move(attrs); }

	const std::string &lEntityId() const noexcept { return _data.lEntityId; }
	const std::string &lEntityType() const noexcept { return _data.lEntityType; }

	const pid_t lPrincipalId() const noexcept {
		if (_data.lEntityType == common::principal_entity_v) {
			return _data.lEntityId;
		}

		return {};
	}

	void lPrincipalId(const std::string &pid) noexcept {
		_data.lEntityId   = pid;
		_data.lEntityType = common::principal_entity_v;

		hash();
	}

	const std::string &relation() const noexcept { return _data.relation; }

	const std::string &rEntityId() const noexcept { return _data.rEntityId; }
	const std::string &rEntityType() const noexcept { return _data.rEntityType; }

	const pid_t rPrincipalId() const noexcept {
		if (_data.rEntityType == common::principal_entity_v) {
			return _data.rEntityId;
		}

		return {};
	}

	void rPrincipalId(const std::string &pid) noexcept {
		_data.rEntityId   = pid;
		_data.rEntityType = common::principal_entity_v;

		hash();
	}

	const std::string &spaceId() const noexcept { return _data.spaceId; }

	const std::string &strand() const noexcept { return _data.strand; }
	void               strand(const std::string &strand) noexcept { _data.strand = strand; }

	const std::string &id() const noexcept { return _id; }
	const int         &rev() const noexcept { return _rev; }

	const std::int64_t lHash() const noexcept { return _lHash; }
	const std::int64_t rHash() const noexcept { return _rHash; }

	const rid_t &ridL() const noexcept { return _ridL; }
	const rid_t &ridR() const noexcept { return _ridR; }

	void store();

	static bool discard(std::string_view spaceId, std::string_view id);

	static std::optional<Tuple> lookup(
		std::string_view spaceId, Entity left, Entity right, std::string_view relation = "",
		std::string_view strand = "");

	static Tuple retrieve(std::string_view id);

private:
	void hash() noexcept;

	Data         _data;
	std::string  _id;
	int          _rev;
	std::int64_t _lHash, _rHash;
	rid_t        _ridL, _ridR;
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
