include(FetchContent)

# glaze
FetchContent_Declare(glaze
	URL      https://github.com/stephenberry/glaze/archive/refs/tags/v1.2.6.tar.gz
	URL_HASH SHA256=ef602f1efc7f84669de517181cb091c136e2b9372c176947d0940ebd6c2f2d98
)
FetchContent_MakeAvailable(glaze)

# grpc & protobuf
if (GATEKEEPER_FAVOUR_SYSTEM_GRPC)
	find_package(gRPC 1.48.0)
endif()

if (gRPC_FOUND)
	message(STATUS "Using gRPC v${gRPC_VERSION}")

	find_package(Protobuf REQUIRED)
	find_program(gRPC_CPP_PLUGIN_EXECUTABLE
		grpc_cpp_plugin
		DOC "gRPC C++ plugin for protoc"
	)
else()
	message(STATUS "Fetching gRPC and dependencies (this can take a while)")
	FetchContent_Declare(grpc
		GIT_REPOSITORY https://github.com/grpc/grpc
		GIT_TAG        v1.48.4
	)

	set(gRPC_BUILD_GRPC_CSHARP_PLUGIN      OFF CACHE BOOL "Build gRPC C# plugin")
	set(gRPC_BUILD_GRPC_NODE_PLUGIN        OFF CACHE BOOL "Build gRPC Node plugin")
	set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE BOOL "Build gRPC Objective-C plugin")
	set(gRPC_BUILD_GRPC_PHP_PLUGIN         OFF CACHE BOOL "Build gRPC PHP plugin")
	set(gRPC_BUILD_GRPC_PYTHON_PLUGIN      OFF CACHE BOOL "Build gRPC Python plugin")
	set(gRPC_BUILD_GRPC_RUBY_PLUGIN        OFF CACHE BOOL "Build gRPC Ruby plugin")

	FetchContent_MakeAvailable(grpc)

	# Ensure `FetchContent` outputs are aligned with `find_package` outputs
	set(Protobuf_PROTOC_EXECUTABLE $<TARGET_FILE:protobuf::protoc>)
	get_target_property(Protobuf_INCLUDE_DIR protobuf::libprotoc INTERFACE_INCLUDE_DIRECTORIES)

	add_library(gRPC::grpc++ ALIAS grpc++)
	set(gRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

	message(STATUS "Fetching gRPC and dependencies - done")
endif()

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

# hiredis
FetchContent_Declare(hiredis
	URL      https://github.com/redis/hiredis/archive/refs/tags/v1.1.0.tar.gz
	URL_HASH SHA256=fe6d21741ec7f3fc9df409d921f47dfc73a4d8ff64f4ac6f1d95f951bf7f53d6
)

set(DISABLE_TESTS ON CACHE BOOL "Disable tests")
FetchContent_MakeAvailable(hiredis)

# libpqxx
FetchContent_Declare(libpqxx
	URL      https://github.com/jtv/libpqxx/archive/refs/tags/7.7.5.tar.gz
	URL_HASH SHA256=c7dc3e8fa2eee656f2b6a8179d72f15db10e97a80dc4f173f806e615ea990973
)

set(SKIP_BUILD_TEST ON) # Not defined as `option()`, no need to set as a cache entry (Ref: CMP0077)
FetchContent_MakeAvailable(libpqxx)

# libviper
FetchContent_Declare(libviper
	URL      https://github.com/uditha-atukorala/libviper/archive/refs/tags/v0.3.1.tar.gz
	URL_HASH SHA256=24a79fe54708a315394938f7946dab35aa69b884f0fab4ac0a9fa42b60c93313
)
FetchContent_MakeAvailable(libviper)

#libxid
FetchContent_Declare(libxid
	URL      https://github.com/uditha-atukorala/libxid/archive/refs/tags/v0.1.0.tar.gz
	URL_HASH SHA256=31589bb5274c9d25a8b6c49ee04a6c76151f10082e7feb13314be02a4b2d58c8
)
FetchContent_MakeAvailable(libxid)

