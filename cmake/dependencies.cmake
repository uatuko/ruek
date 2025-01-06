include(FetchContent)

# protobuf
find_package(Protobuf REQUIRED)

# fmt
FetchContent_Declare(fmt
	URL      https://github.com/fmtlib/fmt/archive/refs/tags/11.1.1.tar.gz
	URL_HASH SHA256=482eed9efbc98388dbaee5cb5f368be5eca4893456bb358c18b7ff71f835ae43
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
	URL      https://github.com/jtv/libpqxx/archive/refs/tags/7.10.0.tar.gz
	URL_HASH SHA256=d588bca36357eda8bcafd5bc1f95df1afe613fdc70c80e426fc89eecb828fc3e
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
		URL      https://github.com/google/benchmark/archive/refs/tags/v1.9.1.tar.gz
		URL_HASH SHA256=32131c08ee31eeff2c8968d7e874f3cb648034377dfc32a4c377fa8796d84981
	)

	set(BENCHMARK_ENABLE_TESTING     OFF CACHE BOOL "Disable tests for google benchmark")
	set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable google benchmark unit tests")
	FetchContent_MakeAvailable(benchmark)
endif()

if (RUEK_BUILD_TESTING)
	# googletest
	FetchContent_Declare(googletest
		URL      https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
		URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
		FIND_PACKAGE_ARGS 1.15.2 NAMES GTest
	)
	FetchContent_MakeAvailable(googletest)
endif()
