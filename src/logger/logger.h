#include <string>

#include <glaze/glaze.hpp>

namespace logger {
std::string timestamp();

template <typename... Args> void info(std::string_view source, Args &&...args) {
	glz::obj obj{
		"@timestamp",
		timestamp(),
		"severity",
		"info",
		"source",
		source,
		args...,
	};

	std::cout << glz::write_json(obj) << std::endl;
}
} // namespace logger
