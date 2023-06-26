#include "publisher.h"

namespace events {
void Publisher::send(const std::string &data) {
	if (!_transport) {
		return;
	}

	_transport(data);
}
} // namespace events
