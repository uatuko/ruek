#pragma once

#include <chrono>
#include <string>

using namespace std::chrono_literals;

namespace datastore {
struct config {
	struct duration_t : public std::chrono::nanoseconds {
		duration_t(const std::chrono::milliseconds &ms) : std::chrono::nanoseconds(ms) {}

		explicit operator timeval() const noexcept {
			auto dur = *this;

			auto secs = std::chrono::duration_cast<std::chrono::seconds>(dur);
			dur -= secs;

			auto micros = std::chrono::duration_cast<std::chrono::microseconds>(dur);

			return {
				.tv_sec  = secs.count(),
				.tv_usec = static_cast<suseconds_t>(micros.count()),
			};
		}
	};

	struct pg_t {
		std::string opts;
	};

	struct redis_t {
		std::string host = "localhost";
		int         port = 6379;

		duration_t timeout = 500ms;
	};

	pg_t    pg;
	redis_t redis;
};
} // namespace datastore
