#include <benchmark/benchmark.h>
#include <grpcxx/request.h>
#include <xid/xid.h>

#include "db/testing.h"
#include "svc/svc.h"

using namespace sentium::api::v1::Relations;

class bm_relations : public benchmark::Fixture {
public:
	void SetUp(benchmark::State &state) {
		db::testing::setup();

		// Clear data
		db::pg::exec("truncate table tuples;");
	}

	void TearDown(benchmark::State &state) { db::testing::teardown(); }
};

// Attempts to create as many direct relations as possible.
// e.g. []user:jane/member/group:viewers
BENCHMARK_F(bm_relations, create)(benchmark::State &st) {
	grpcxx::context ctx;
	svc::Relations  svc;

	rpcCreate::request_type request;
	rpcCreate::result_type  result;

	auto *left = request.mutable_left_entity();
	left->set_type("user");

	request.set_relation("member");

	auto *right = request.mutable_right_entity();
	right->set_id("bm_relations.create");
	right->set_type("group");

	std::size_t ops  = 0;
	std::size_t cost = 0;
	for (auto _ : st) {
		st.PauseTiming();
		left->set_id(xid::next());
		ops++;
		st.ResumeTiming();

		result = svc.call<rpcCreate>(ctx, request);

		st.PauseTiming();
		cost += result.response->cost();
		st.ResumeTiming();
	}

	st.counters.insert({
		{"ops", benchmark::Counter(ops, benchmark::Counter::kIsRate)},
		{"writes", benchmark::Counter(cost, benchmark::Counter::kIsRate)},
	});
}

// Benchmark optimise algorithm for a simple use case with different numbers of pre-existing
// relations. e.g.
//       strand |  l_entity_id  | relation |  r_entity_id
//      --------+---------------+----------+---------------
//              | user:jane     | member   | group:editors     <- already exists
//       member | group:editors | parent   | group:viewers     <- create
//              | user:jane     | parent   | group:viewers     <- compute
BENCHMARK_DEFINE_F(bm_relations, optimise)(benchmark::State &st) {
	for (int n = st.range(0); n > 0; n--) {
		db::Tuple tuple({
			.lEntityId   = xid::next(),
			.lEntityType = "user",
			.relation    = "member",
			.rEntityId   = "bm_relations.optimise",
			.rEntityType = "group",
		});

		try {
			tuple.store();
		} catch (const std::exception &e) {
			st.SkipWithError(e.what());
			break;
		}
	}

	grpcxx::context ctx;
	svc::Relations  svc;

	rpcCreate::request_type request;
	request.set_optimise(true);
	request.set_cost_limit(std::numeric_limits<std::uint16_t>::max());

	auto *left = request.mutable_left_entity();
	left->set_id("bm_relations.optimise");
	left->set_type("group");

	request.set_relation("parent");

	auto *right = request.mutable_right_entity();
	right->set_type("bm_relations.optimise");

	request.set_strand("member");

	std::size_t ops  = 0;
	std::size_t cost = 0;

	rpcCreate::result_type result;

	for (auto _ : st) {
		st.PauseTiming();
		right->set_id(xid::next());
		ops++;
		st.ResumeTiming();

		result = svc.call<rpcCreate>(ctx, request);

		st.PauseTiming();
		cost += result.response->cost();
		st.ResumeTiming();
	}

	st.counters.insert({
		{"ops", benchmark::Counter(ops, benchmark::Counter::kIsRate)},
		{"writes", benchmark::Counter(cost, benchmark::Counter::kIsRate)},
	});
}
BENCHMARK_REGISTER_F(bm_relations, optimise)->Range(8, 2 << 10);
