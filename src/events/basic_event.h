#pragma once

#include <chrono>
#include <string>

#include <glaze/glaze.hpp>

#include "concepts.h"

namespace events {
template <encodable T> struct basic_event {
	using clock_t   = std::chrono::system_clock;
	using payload_t = T;

	struct timestamp_t : public clock_t::time_point {
		timestamp_t(const clock_t::time_point &tp) : clock_t::time_point(tp) {}
		timestamp_t(clock_t::time_point &&tp) : clock_t::time_point(std::move(tp)) {}

		std::string rfc3339nano() const {
			const auto                  tp   = *this;
			auto                        days = std::chrono::floor<std::chrono::days>(tp);
			std::chrono::year_month_day ymd(days);
			std::chrono::hh_mm_ss time(std::chrono::floor<std::chrono::nanoseconds>(tp - days));

			char buf[std::size("2006-01-02T15:04:05.999999999Z")];
			std::snprintf(
				std::data(buf),
				std::size(buf),
				"%d-%.2u-%.2uT%.2ld:%.2ld:%.2lld.%lldZ",
				static_cast<int>(ymd.year()),
				static_cast<unsigned>(ymd.month()),
				static_cast<unsigned>(ymd.day()),
				time.hours().count(),
				time.minutes().count(),
				time.seconds().count(),
				time.subseconds().count());

			return buf;
		}
	};

	std::string name;
	payload_t   payload;
	timestamp_t timestamp = std::chrono::system_clock::now();

	std::string serialize() const {
		auto obj = glz::obj{
			"name", name, "payload", payload.encode(), "timestamp", timestamp.rfc3339nano()};

		return glz::write_json(obj);
	}
};
} // namespace events
