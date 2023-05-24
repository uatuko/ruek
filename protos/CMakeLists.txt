# libprotos
cmake_path(SET gatekeeper_proto ${CMAKE_CURRENT_SOURCE_DIR}/gk/v1/gatekeeper.proto)
cmake_path(SET gatekeeper_grpc_header ${CMAKE_CURRENT_BINARY_DIR}/gk/v1/gatekeeper.grpc.pb.h)
cmake_path(SET gatekeeper_grpc_source ${CMAKE_CURRENT_BINARY_DIR}/gk/v1/gatekeeper.grpc.pb.cc)
cmake_path(SET gatekeeper_header ${CMAKE_CURRENT_BINARY_DIR}/gk/v1/gatekeeper.pb.h)
cmake_path(SET gatekeeper_source ${CMAKE_CURRENT_BINARY_DIR}/gk/v1/gatekeeper.pb.cc)

set(protos
	${gatekeeper_proto}
)

set(headers
	${gatekeeper_grpc_header}
	${gatekeeper_header}
)

set(sources
	${gatekeeper_grpc_source}
	${gatekeeper_source}
)

add_custom_command(
	OUTPUT ${headers} ${sources}
	DEPENDS ${protos}
	COMMAND $<TARGET_FILE:protoc>
	ARGS
		--proto_path=${CMAKE_CURRENT_SOURCE_DIR}
		--proto_path=${googleapis_SOURCE_DIR}
		--proto_path=${protobuf_include_dir}
		--cpp_out=${CMAKE_CURRENT_BINARY_DIR}
		--grpc_out=${CMAKE_CURRENT_BINARY_DIR}
		--plugin=protoc-gen-grpc=$<TARGET_FILE:grpc_cpp_plugin>
		${protos}
)

add_library(protos
	${sources}
)

target_include_directories(protos
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(protos
	googleapis
	grpc++
)

add_library(${PROJECT_NAME}::libprotos ALIAS protos)

