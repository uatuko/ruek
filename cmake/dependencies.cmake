include(FetchContent)

# protobuf
find_package(Protobuf REQUIRED)

# fmt
FetchContent_Declare(fmt
	URL      https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.tar.gz
	URL_HASH SHA256=78b8c0a72b1c35e4443a7e308df52498252d1cefc2b08c9a97bc9ee6cfe61f8b
)
FetchContent_MakeAvailable(fmt)

# grpcxx
FetchContent_Declare(grpcxx
	URL      https://github.com/uatuko/grpcxx/archive/refs/tags/v0.2.0.tar.gz
	URL_HASH SHA256=ed0e0c6ccd44aabb9447de9030b9be092bfb97cb654d69c970cbefd7b7bb44da
)
FetchContent_MakeAvailable(grpcxx)

# libpqxx
FetchContent_Declare(libpqxx
	URL      https://github.com/jtv/libpqxx/archive/refs/tags/7.7.5.tar.gz
	URL_HASH SHA256=c7dc3e8fa2eee656f2b6a8179d72f15db10e97a80dc4f173f806e615ea990973
)

set(SKIP_BUILD_TEST ON) # Not defined as `option()`, no need to set as a cache entry (Ref: CMP0077)
FetchContent_MakeAvailable(libpqxx)

#libxid
FetchContent_Declare(libxid
	URL      https://github.com/uditha-atukorala/libxid/archive/refs/tags/v0.1.0.tar.gz
	URL_HASH SHA256=31589bb5274c9d25a8b6c49ee04a6c76151f10082e7feb13314be02a4b2d58c8
)
FetchContent_MakeAvailable(libxid)


if (RUEK_BUILD_BENCHMARKS)
	# google benchmark
	FetchContent_Declare(benchmark
		URL      https://github.com/google/benchmark/archive/refs/tags/v1.8.3.tar.gz
		URL_HASH SHA256=6bc180a57d23d4d9515519f92b0c83d61b05b5bab188961f36ac7b06b0d9e9ce
	)

	set(BENCHMARK_ENABLE_TESTING     OFF CACHE BOOL "Disable tests for google benchmark")
	set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable google benchmark unit tests")
	FetchContent_MakeAvailable(benchmark)
endif()

if (RUEK_BUILD_TESTING)
	# googletest
	FetchContent_Declare(googletest
		URL      https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
		URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
		FIND_PACKAGE_ARGS NAMES GTest
	)
	FetchContent_MakeAvailable(googletest)
endif()
