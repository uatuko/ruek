#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "pg.h"
#include "tuples.h"

namespace db {
class Tuplet {
public:
	using strand_t = std::optional<std::string>;

	Tuplet(const pg::row_t &r);

	const std::int64_t &hash() const noexcept { return _hash; }
	const std::string  &id() const noexcept { return _id; }
	const std::string  &relation() const noexcept { return _relation; }
	const strand_t     &strand() const noexcept { return _strand; }

private:
	std::int64_t _hash;
	std::string  _id;
	std::string  _relation;
	strand_t     _strand;
};

using Tuplets = std::vector<Tuplet>;

Tuplets TupletsList(
	std::string_view spaceId, std::optional<Tuple::Entity> left, std::optional<Tuple::Entity> right,
	std::optional<std::string_view> relation = std::nullopt, std::uint16_t count = 10);
} // namespace db
