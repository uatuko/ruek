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

	std::size_t ops = 0;
	for (auto _ : st) {
		st.PauseTiming();
		ops++;
		st.ResumeTiming();

		for (const auto &s : left) {
			if (std::binary_search(right.begin(), right.end(), s)) {
				break;
			}
		}
	}

	st.counters.insert({
		{"ops", benchmark::Counter(ops, benchmark::Counter::kIsRate)},
	});
})
	->Name("bm_binary_intersection")
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

	std::size_t              ops = 0;
	std::vector<std::string> intersection;

	for (auto _ : st) {
		st.PauseTiming();
		ops++;
		intersection.clear();
		st.ResumeTiming();

		std::set_intersection(
			left.begin(), left.end(), right.begin(), right.end(), std::back_inserter(intersection));
	}

	st.counters.insert({
		{"ops", benchmark::Counter(ops, benchmark::Counter::kIsRate)},
	});
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

	std::size_t ops  = 0;
	std::size_t cost = 0;
	for (auto _ : st) {
		ops++;

		auto i = left.cbegin();
		auto j = right.cbegin();

		while (i != left.cend() && j != right.cend()) {
			cost++;
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

	st.counters.insert({
		{"ops", benchmark::Counter(ops, benchmark::Counter::kIsRate)},
		{"comparisons", benchmark::Counter(cost, benchmark::Counter::kIsRate)},
	});
})
	->Name("bm_spot_intersection")
	->Range(8, 8 << 10);

BENCHMARK([](benchmark::State &st) {
	std::vector<std::int64_t> left;
	std::vector<std::int64_t> right;

	left.reserve(st.range(0));
	right.reserve(st.range(0));

	for (int n = st.range(0); n > 0; n--) {
		left.emplace_back(std::rand());
		right.emplace_back(std::rand());
	}

	std::sort(left.begin(), left.end());
	std::sort(right.begin(), right.end());

	std::size_t ops  = 0;
	std::size_t cost = 0;
	for (auto _ : st) {
		ops++;

		auto i = left.cbegin();
		auto j = right.cbegin();

		while (i != left.cend() && j != right.cend()) {
			cost++;

			if (*i == *j) {
				break;
			}

			if (*i < *j) {
				i++;
			} else {
				j++;
			}
		}
	}

	st.counters.insert({
		{"ops", benchmark::Counter(ops, benchmark::Counter::kIsRate)},
		{"comparisons", benchmark::Counter(cost, benchmark::Counter::kIsRate)},
	});
})
	->Name("bm_spot_intersection_int64")
	->Range(8, 8 << 10);
