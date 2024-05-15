#pragma once

#include <string_view>

namespace svc {
namespace common {
enum struct strategy_t : std::uint32_t {
	unknown = 0,
	direct  = 2,
	graph   = 4,
	set     = 8,
};

static constexpr std::uint16_t cost_limit_v = 1000;

static constexpr std::uint16_t pagination_limit_v = 30;

static constexpr std::string_view space_id_v = "space-id";
} // namespace common
} // namespace svc
