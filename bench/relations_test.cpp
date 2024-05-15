#include <benchmark/benchmark.h>
#include <grpcxx/request.h>
#include <xid/xid.h>

#include "db/testing.h"
#include "svc/common.h"
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

// Benchmark optimize algorithm for a simple use case with different numbers of pre-existing
// relations. e.g.
//     strand |  l_entity_id  | relation |  r_entity_id
//     -------+---------------+----------+---------------
//            | user:jane     | member   | group:editors     <- already exists
//     member | group:editors | parent   | group:viewers     <- create
//            | user:jane     | parent   | group:viewers     <- compute
BENCHMARK_DEFINE_F(bm_relations, optimize)(benchmark::State &st) {
	for (int n = st.range(0); n > 0; n--) {
		db::Tuple tuple({
			.lEntityId   = xid::next(),
			.lEntityType = "user",
			.relation    = "member",
			.rEntityId   = "bm_relations.optimize",
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
	request.set_optimize(static_cast<std::uint32_t>(svc::common::strategy_t::direct));
	request.set_cost_limit(std::numeric_limits<std::uint16_t>::max());

	auto *left = request.mutable_left_entity();
	left->set_id("bm_relations.optimize");
	left->set_type("group");

	request.set_relation("parent");

	auto *right = request.mutable_right_entity();
	right->set_type("bm_relations.optimize");

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
BENCHMARK_REGISTER_F(bm_relations, optimize)->Range(8, 2 << 10);

// Benchmark optimize algorithm for creating deeply nested relations. e.g.
//      strand  |  l_entity_id  | relation |  r_entity_id
//     ---------+---------------+----------+---------------
//              | group:admins  | admins   | group:editors     <- create(1)
//      admin   | group:editors | editors  | group:writers     <- create(2)
//      editors | group:writers | readers  | group:readers     <- create(3)
//              | group:admins  | editors  | group:writers     <- compute(2)
//              | group:editors | readers  | group:readers     <- compute(3)
BENCHMARK_DEFINE_F(bm_relations, optimize_nested)(benchmark::State &st) {
	grpcxx::context ctx;
	svc::Relations  svc;

	std::size_t            ops  = 0;
	std::size_t            cost = 0;
	rpcCreate::result_type result;

	for (auto _ : st) {
		st.PauseTiming();
		rpcCreate::request_type request;
		request.set_optimize(static_cast<std::uint32_t>(svc::common::strategy_t::direct));
		request.set_cost_limit(std::numeric_limits<std::uint16_t>::max());

		auto *left = request.mutable_left_entity();
		left->set_id(xid::next());
		left->set_type("group");

		request.set_relation("member");

		auto *right = request.mutable_right_entity();
		right->set_id(xid::next());
		right->set_type("group");

		ops++;
		st.ResumeTiming();

		result = svc.call<rpcCreate>(ctx, request);

		st.PauseTiming();
		cost += result.response->cost();
		st.ResumeTiming();

		for (int n = st.range(0); n > 0; n--) {
			st.PauseTiming();
			auto &tuple = result.response->tuple();
			auto *left  = request.mutable_left_entity();
			left->set_id(tuple.right_entity().id());
			left->set_type(tuple.right_entity().type());

			auto *right = request.mutable_right_entity();
			right->set_id(xid::next());
			right->set_type("group");

			request.set_strand(tuple.relation());

			ops++;
			st.ResumeTiming();

			result = svc.call<rpcCreate>(ctx, request);

			st.PauseTiming();
			cost += result.response->cost();
			st.ResumeTiming();
		}
	}

	st.counters.insert({
		{"ops", benchmark::Counter(ops, benchmark::Counter::kIsRate)},
		{"writes", benchmark::Counter(cost, benchmark::Counter::kIsRate)},
	});
}
BENCHMARK_REGISTER_F(bm_relations, optimize_nested)->Range(8, 512);
