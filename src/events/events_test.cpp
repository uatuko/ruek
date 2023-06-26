#include <gtest/gtest.h>

#include "events.h"

struct payload {
	using data_t = std::map<std::string, std::variant<std::string, bool, int>>;

	std::string type = "test";

	data_t data = {
		{"bool", false},
		{"int", 45},
		{"str", "string of data"},
	};

	auto encode() const { return glz::obj{"type", type, "data", data}; }
};

TEST(events, publish) {
	// Success: publish event
	{
		std::string                       serializedData;
		events::Publisher::transport_type transport = [&serializedData](const std::string &data) {
			serializedData = data;
		};

		auto &p = events::publisher();
		p.transport(transport);

		std::chrono::microseconds                          us(1'687'668'087'461'048);
		std::chrono::time_point<std::chrono::system_clock> tp(us);

		events::basic_event<payload> e{
			.name      = "name:events.publish",
			.timestamp = tp,
		};
		events::publish(e);

		std::string expected =
			R"({"name":"name:events.publish","payload":{"type":"test","data":{"bool":false,"int":45,"str":"string of data"}},"timestamp":"2023-06-25T04:41:27.461048000Z"})";
		EXPECT_EQ(expected, serializedData);
	}
}
