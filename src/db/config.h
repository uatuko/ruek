#pragma once

#include <chrono>
#include <string>

using namespace std::chrono_literals;

namespace db {
struct config {
	struct duration_t : public std::chrono::nanoseconds {
		duration_t(const std::chrono::milliseconds &ms) : std::chrono::nanoseconds(ms) {}

		explicit operator timeval() const noexcept {
			auto dur = *this;

			auto secs  = std::chrono::duration_cast<std::chrono::seconds>(dur);
			dur       -= secs;

			auto micros = std::chrono::duration_cast<std::chrono::microseconds>(dur);

			return {
				.tv_sec  = secs.count(),
				.tv_usec = static_cast<suseconds_t>(micros.count()),
			};
		}
	};

	std::string opts;
	duration_t  timeout = 1000ms;
};
} // namespace db
