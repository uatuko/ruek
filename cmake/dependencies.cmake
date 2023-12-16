include(FetchContent)

# protobuf
find_package(Protobuf REQUIRED)

# grpcxx
FetchContent_Declare(grpcxx
	URL      https://github.com/uatuko/grpcxx/archive/c6934c3223a76f50439bb1dda98aa25482829b95.tar.gz
	URL_HASH SHA256=df156fee18d1b96912d96a460d4fb3515f04630813ad6a6ad41bb9f06e014bb4
)
FetchContent_MakeAvailable(grpcxx)

# googleapis
FetchContent_Declare(googleapis
	URL      https://github.com/googleapis/googleapis/archive/0e3b813b0d0da539eacbe86b8716feeed00943c5.tar.gz
	URL_HASH SHA256=44f3b9c73a5df760c4fad3cf0c5cc54732b881f00708308f7635ff75a13dcaa5
)
FetchContent_MakeAvailable(googleapis)

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
