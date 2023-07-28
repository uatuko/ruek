#include <string>

#include <glaze/glaze.hpp>

namespace logger {
std::string timestamp();

template <typename... Args>
inline void log(std::string_view severity, std::string_view source, Args &&...args) {
	glz::obj obj{
		"@timestamp",
		timestamp(),
		"severity",
		severity,
		"source",
		source,
		args...,
	};

	std::cout << glz::write_json(obj) << std::endl;
}

void critical(std::string_view source, auto &&...args) {
	log("critical", source, std::forward<decltype(args)>(args)...);
	std::exit(EXIT_FAILURE);
}

inline void info(std::string_view source, auto &&...args) {
	log("info", source, std::forward<decltype(args)>(args)...);
}
} // namespace logger
