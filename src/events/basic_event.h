#pragma once

#include <chrono>
#include <string>

#include <glaze/glaze.hpp>

#include "concepts.h"

namespace events {
template <encodable T> struct basic_event {
	using payload_t   = T;
	using timestamp_t = std::chrono::sys_time<std::chrono::nanoseconds>;

	std::string name;
	payload_t   payload;
	timestamp_t timestamp;

	std::string serialize() const {
		auto obj = glz::obj{"name", name, "payload", payload.encode()};

		return glz::write_json(obj);
	}
};
} // namespace events
