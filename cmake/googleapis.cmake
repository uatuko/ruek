cmake_path(SET googleapis_annotations_proto ${googleapis_SOURCE_DIR}/google/api/annotations.proto)
cmake_path(SET googleapis_annotations_header ${CMAKE_CURRENT_BINARY_DIR}/google/api/annotations.pb.h)
cmake_path(SET googleapis_annotations_source ${CMAKE_CURRENT_BINARY_DIR}/google/api/annotations.pb.cc)

cmake_path(SET googleapis_http_proto ${googleapis_SOURCE_DIR}/google/api/http.proto)
cmake_path(SET googleapis_http_header ${CMAKE_CURRENT_BINARY_DIR}/google/api/http.pb.h)
cmake_path(SET googleapis_http_source ${CMAKE_CURRENT_BINARY_DIR}/google/api/http.pb.cc)

cmake_path(SET googleapis_rpc_code_proto ${googleapis_SOURCE_DIR}/google/rpc/code.proto)
cmake_path(SET googleapis_rpc_code_header ${CMAKE_CURRENT_BINARY_DIR}/google/rpc/code.pb.h)
cmake_path(SET googleapis_rpc_code_source ${CMAKE_CURRENT_BINARY_DIR}/google/rpc/code.pb.cc)

cmake_path(SET googleapis_rpc_status_proto ${googleapis_SOURCE_DIR}/google/rpc/status.proto)
cmake_path(SET googleapis_rpc_status_header ${CMAKE_CURRENT_BINARY_DIR}/google/rpc/status.pb.h)
cmake_path(SET googleapis_rpc_status_source ${CMAKE_CURRENT_BINARY_DIR}/google/rpc/status.pb.cc)

set(googleapis_protos
	${googleapis_annotations_proto}
	${googleapis_http_proto}
	${googleapis_rpc_code_proto}
	${googleapis_rpc_status_proto}
)

set(googleapis_headers
	${googleapis_annotations_header}
	${googleapis_http_header}
	${googleapis_rpc_code_header}
	${googleapis_rpc_status_header}
)

set(googleapis_sources
	${googleapis_annotations_source}
	${googleapis_http_source}
	${googleapis_rpc_code_source}
	${googleapis_rpc_status_source}
)

add_custom_command(
	OUTPUT ${googleapis_headers} ${googleapis_sources}
	DEPENDS ${googleapis_protos}
	COMMAND ${Protobuf_PROTOC_EXECUTABLE}
	ARGS
		--proto_path=${googleapis_SOURCE_DIR}
		--proto_path=${Protobuf_INCLUDE_DIR}
		--cpp_out=${CMAKE_CURRENT_BINARY_DIR}
		${googleapis_protos}
)

add_library(googleapis
	${googleapis_sources}
)

target_include_directories(googleapis
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
	PRIVATE ${Protobuf_INCLUDE_DIR}
)
