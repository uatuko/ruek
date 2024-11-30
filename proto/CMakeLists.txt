# ruek/api/v1/**/*.proto
cmake_path(SET principals_proto  ${CMAKE_CURRENT_SOURCE_DIR}/ruek/api/v1/principals.proto)
cmake_path(SET principals_grpcxx_header ${CMAKE_CURRENT_BINARY_DIR}/ruek/api/v1/principals.grpcxx.pb.h)
cmake_path(SET principals_header ${CMAKE_CURRENT_BINARY_DIR}/ruek/api/v1/principals.pb.h)
cmake_path(SET principals_source ${CMAKE_CURRENT_BINARY_DIR}/ruek/api/v1/principals.pb.cc)

cmake_path(SET relations_proto  ${CMAKE_CURRENT_SOURCE_DIR}/ruek/api/v1/relations.proto)
cmake_path(SET relations_grpcxx_header ${CMAKE_CURRENT_BINARY_DIR}/ruek/api/v1/relations.grpcxx.pb.h)
cmake_path(SET relations_header ${CMAKE_CURRENT_BINARY_DIR}/ruek/api/v1/relations.pb.h)
cmake_path(SET relations_source ${CMAKE_CURRENT_BINARY_DIR}/ruek/api/v1/relations.pb.cc)

set(protos
	${principals_proto}
	${relations_proto}
)

set(headers
	${principals_header} ${principals_grpcxx_header}
	${relations_header} ${relations_grpcxx_header}
)

set(sources
	${principals_source}
	${relations_source}
)

add_custom_command(
	OUTPUT  ${headers} ${sources}
	DEPENDS ${protos}
	COMMAND ${Protobuf_PROTOC_EXECUTABLE}
	ARGS
		--proto_path=${CMAKE_CURRENT_SOURCE_DIR}
		--proto_path=${Protobuf_INCLUDE_DIR}
		--cpp_out=${CMAKE_CURRENT_BINARY_DIR}
		--grpcxx_out=${CMAKE_CURRENT_BINARY_DIR}
		--plugin=protoc-gen-grpcxx=$<TARGET_FILE:libgrpcxx::protoc-gen>
		${protos}
)


# ruek/detail/**/*.proto
cmake_path(SET detail_pagination_proto ${CMAKE_CURRENT_SOURCE_DIR}/ruek/detail/pagination.proto)
cmake_path(SET detail_pagination_header ${CMAKE_CURRENT_BINARY_DIR}/ruek/detail/pagination.pb.h)
cmake_path(SET detail_pagination_source ${CMAKE_CURRENT_BINARY_DIR}/ruek/detail/pagination.pb.cc)

set(detail_protos
	${detail_pagination_proto}
)

set(detail_headers
	${detail_pagination_header}
)

set(detail_sources
	${detail_pagination_source}
)

add_custom_command(
	OUTPUT  ${detail_headers} ${detail_sources}
	DEPENDS ${detail_protos}
	COMMAND ${Protobuf_PROTOC_EXECUTABLE}
	ARGS
		--proto_path=${CMAKE_CURRENT_SOURCE_DIR}
		--proto_path=${Protobuf_INCLUDE_DIR}
		--cpp_out=${CMAKE_CURRENT_BINARY_DIR}
		${detail_protos}
)


# google/rpc/**/*.proto
cmake_path(SET google_rpc_code_proto  ${CMAKE_CURRENT_SOURCE_DIR}/google/rpc/code.proto)
cmake_path(SET google_rpc_code_header ${CMAKE_CURRENT_BINARY_DIR}/google/rpc/code.pb.h)
cmake_path(SET google_rpc_code_source ${CMAKE_CURRENT_BINARY_DIR}/google/rpc/code.pb.cc)

cmake_path(SET google_rpc_status_proto  ${CMAKE_CURRENT_SOURCE_DIR}/google/rpc/status.proto)
cmake_path(SET google_rpc_status_header ${CMAKE_CURRENT_BINARY_DIR}/google/rpc/status.pb.h)
cmake_path(SET google_rpc_status_source ${CMAKE_CURRENT_BINARY_DIR}/google/rpc/status.pb.cc)

set(google_protos
	${google_rpc_code_proto}
	${google_rpc_status_proto}
)

set(google_headers
	${google_rpc_code_header}
	${google_rpc_status_header}
)

set(google_sources
	${google_rpc_code_source}
	${google_rpc_status_source}
)

add_custom_command(
	OUTPUT  ${google_headers} ${google_sources}
	DEPENDS ${google_protos}
	COMMAND ${Protobuf_PROTOC_EXECUTABLE}
	ARGS
		--proto_path=${CMAKE_CURRENT_SOURCE_DIR}
		--proto_path=${Protobuf_INCLUDE_DIR}
		--cpp_out=${CMAKE_CURRENT_BINARY_DIR}
		${google_protos}
)


add_library(proto
	${sources}
	${detail_sources}
	${google_sources}
)

target_include_directories(proto
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(proto
	libgrpcxx::grpcxx
	protobuf::libprotobuf
)

add_library(${PROJECT_NAME}::libproto ALIAS proto)
