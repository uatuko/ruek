#include "log.h"

#include <chrono>

namespace log {
std::string timestamp() {
	const auto                  now  = std::chrono::system_clock::now();
	auto                        days = std::chrono::floor<std::chrono::days>(now);
	std::chrono::year_month_day ymd(days);
	std::chrono::hh_mm_ss       time(std::chrono::floor<std::chrono::nanoseconds>(now - days));

	char buf[std::size("2006-01-02T15:04:05.999999999Z")];
	std::snprintf(
		std::data(buf),
		std::size(buf),
		"%d-%.2u-%.2uT%.2ld:%.2ld:%.2lld.%lldZ",
		static_cast<int>(ymd.year()),
		static_cast<unsigned>(ymd.month()),
		static_cast<unsigned>(ymd.day()),
		time.hours().count(),
		time.minutes().count(),
		time.seconds().count(),
		time.subseconds().count());

	return buf;
}
} // namespace log
