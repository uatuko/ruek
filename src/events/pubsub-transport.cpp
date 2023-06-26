#include "pubsub-transport.h"

#include "err/errors.h"

namespace events {
PubsubTransport::PubsubTransport(std::shared_ptr<pubsub::PublisherConnection> conn) :
	_publisher(conn) {}

PubsubTransport::PubsubTransport(const std::string &projectId, const std::string &topic) :
	_publisher(pubsub::MakePublisherConnection(pubsub::Topic(projectId, topic))) {}

void PubsubTransport::send(const std::string &msg) {
	auto msgId = _publisher.Publish(pubsub::MessageBuilder().SetData(msg).Build()).get();
	if (!msgId) {
		throw err::EventsPubsubTransportFailure();
	}
}
} // namespace events
