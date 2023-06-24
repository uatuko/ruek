#include <google/cloud/pubsub/mocks/mock_publisher_connection.h>
#include <gtest/gtest.h>

#include "pubsub-transport.h"

class PubsubTransportTest : public ::testing::Test {
protected:
	using conn_t     = google::cloud::pubsub_mocks::MockPublisherConnection;
	using conn_ptr_t = std::shared_ptr<conn_t>;

	PubsubTransportTest() : _conn(std::make_shared<conn_t>()) {}

	conn_ptr_t _conn;
};

TEST_F(PubsubTransportTest, send) {
	EXPECT_CALL(*_conn, Publish)
		.WillOnce([](google::cloud::pubsub::PublisherConnection::PublishParams const &p) {
			EXPECT_EQ("message", p.message.data());
			return google::cloud::make_ready_future(
				google::cloud::StatusOr<std::string>("PubsubTransport.send"));
		});

	events::PubsubTransport t(_conn);
	t.send("message");
}
