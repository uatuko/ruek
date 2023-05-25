include(FetchContent)

# grpc & protobuf
message(STATUS "Fetching gRPC and dependencies (this can take a while)")
FetchContent_Declare(grpc
	GIT_REPOSITORY https://github.com/grpc/grpc
	GIT_TAG        v1.47.5
)
FetchContent_MakeAvailable(grpc)
message(STATUS "Fetching gRPC and dependencies - done")

get_target_property(protobuf_include_dir protobuf::libprotoc INTERFACE_INCLUDE_DIRECTORIES)

# googleapis
FetchContent_Declare(googleapis
	URL      https://github.com/googleapis/googleapis/archive/5d2c0c55cf16534d97eb3405840126113ba1ebbd.tar.gz
	URL_HASH SHA256=8111518a0acd858cc279a82641d623a0e7c1bc4ae69721688260b95ed27b6fff
)
FetchContent_MakeAvailable(googleapis)

# googletest
FetchContent_Declare(googletest
	URL      https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz
	URL_HASH SHA256=ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363
)
FetchContent_MakeAvailable(googletest)

# libpqxx
FetchContent_Declare(libpqxx
	URL      https://github.com/jtv/libpqxx/archive/refs/tags/7.7.5.tar.gz
	URL_HASH SHA256=c7dc3e8fa2eee656f2b6a8179d72f15db10e97a80dc4f173f806e615ea990973
)

set(BUILD_SHARED_LIBS OFF)
set(SKIP_BUILD_TEST ON)
FetchContent_MakeAvailable(libpqxx)

# libviper
FetchContent_Declare(libviper
	URL      https://github.com/uditha-atukorala/libviper/archive/refs/tags/v0.3.1.tar.gz
	URL_HASH SHA256=24a79fe54708a315394938f7946dab35aa69b884f0fab4ac0a9fa42b60c93313
)
FetchContent_MakeAvailable(libviper)

