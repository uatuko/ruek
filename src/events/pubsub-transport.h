#pragma once

#include <google/cloud/pubsub/publisher.h>

namespace events {
namespace pubsub = google::cloud::pubsub;

class PubsubTransport {
public:
	PubsubTransport(std::shared_ptr<pubsub::PublisherConnection> conn);
	PubsubTransport(const std::string &projectId, const std::string &topic);

	void send(const std::string &msg);

private:
	pubsub::Publisher _publisher;
};
} // namespace events
