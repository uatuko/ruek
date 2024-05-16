#include <algorithm>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>
#include <xid/xid.h>

BENCHMARK([](benchmark::State &st) {
	std::vector<std::string> left;
	std::vector<std::string> right;

	left.reserve(st.range(0));
	right.reserve(st.range(0));

	for (int n = st.range(0); n > 0; n--) {
		left.emplace_back(xid::next());
		right.emplace_back(xid::next());
	}

	for (auto _ : st) {
		for (const auto &s : left) {
			if (std::binary_search(right.begin(), right.end(), s)) {
				break;
			}
		}
	}
})
	->Name("bm_binary_intersection")
	->Range(8, 8 << 10);

BENCHMARK([](benchmark::State &st) {
	std::vector<std::string> left;
	std::vector<std::string> right;

	left.reserve(st.range(0));
	right.reserve(st.range(0));

	std::vector<std::string> intersection;

	for (int n = st.range(0); n > 0; n--) {
		left.emplace_back(xid::next());
		right.emplace_back(xid::next());
	}

	for (auto _ : st) {
		intersection.clear();
		std::set_intersection(
			left.begin(), left.end(), right.begin(), right.end(), std::back_inserter(intersection));
	}
})
	->Name("bm_set_intersection")
	->Range(8, 8 << 10);

BENCHMARK([](benchmark::State &st) {
	std::vector<std::string> left;
	std::vector<std::string> right;

	left.reserve(st.range(0));
	right.reserve(st.range(0));

	for (int n = st.range(0); n > 0; n--) {
		left.emplace_back(xid::next());
		right.emplace_back(xid::next());
	}

	for (auto _ : st) {
		auto i = left.cbegin();
		auto j = right.cbegin();

		while (i != left.cend() && j != right.cend()) {
			auto r = i->compare(*j);

			if (r == 0) {
				break;
			}

			if (r < 0) {
				i++;
			} else {
				j++;
			}
		}
	}
})
	->Name("bm_spot_intersection")
	->Range(8, 8 << 10);
