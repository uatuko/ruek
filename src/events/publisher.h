#pragma once

#include <functional>
#include <string>

#include "basic_event.h"

namespace events {
class Publisher {
public:
	template <typename T> using event_type = basic_event<T>;

	using transport_type = std::function<void(const std::string &)>;

	Publisher()                  = default;
	Publisher(const Publisher &) = delete;

	void operator=(const Publisher &) = delete;

	template <typename T> void publish(const event_type<T> &ev) {
		auto data = ev.serialize();
		send(data);
	}

	void send(const std::string &data);

	void transport(const transport_type &transport) noexcept { _transport = transport; }
	void transport(transport_type &&transport) noexcept { _transport = std::move(transport); }

private:
	transport_type _transport;
};
} // namespace events
