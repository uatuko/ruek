#include "events.h"

namespace events {
void init(const Publisher::transport_type &transport) {
	if (!transport) {
		return;
	}

	auto &p = publisher();
	p.transport(transport);
}
} // namespace events
