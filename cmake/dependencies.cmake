include(FetchContent)

# protobuf
find_package(Protobuf REQUIRED)

# fmt
FetchContent_Declare(fmt
	URL      https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.tar.gz
	URL_HASH SHA256=78b8c0a72b1c35e4443a7e308df52498252d1cefc2b08c9a97bc9ee6cfe61f8b
)
FetchContent_MakeAvailable(fmt)

# googleapis
FetchContent_Declare(googleapis
	URL      https://github.com/googleapis/googleapis/archive/0e3b813b0d0da539eacbe86b8716feeed00943c5.tar.gz
	URL_HASH SHA256=44f3b9c73a5df760c4fad3cf0c5cc54732b881f00708308f7635ff75a13dcaa5
)
FetchContent_MakeAvailable(googleapis)

# grpcxx
FetchContent_Declare(grpcxx
	URL      https://github.com/uatuko/grpcxx/archive/refs/tags/v0.1.3.tar.gz
	URL_HASH SHA256=441ca21bed3c0413623440c1608da44e60931631af1dc609c18e4a955f8cb3a5
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


if (SENTIUM_BUILD_TESTING)
	# googletest
	FetchContent_Declare(googletest
		URL      https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
		URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
		FIND_PACKAGE_ARGS NAMES GTest
	)
	FetchContent_MakeAvailable(googletest)
endif()
