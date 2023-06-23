#pragma once

#include "publisher.h"

namespace events {
void init(const Publisher::transport_type &transport = {});

Publisher &publisher() {
	static Publisher p;
	return p;
}

template <typename T> auto publish(const Publisher::event_type<T> &ev) {
	auto &p = publisher();
	return p.publish(ev);
}
} // namespace events
