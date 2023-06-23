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

		events::basic_event<payload> e{
			.name = "name:events.publish",
		};
		events::publish(e);

		std::string expected =
			R"({"name":"name:events.publish","payload":{"type":"test","data":{"bool":false,"int":45,"str":"string of data"}}})";
		EXPECT_EQ(expected, serializedData);
	}
}
