cmake_path(SET authz_proto  ${CMAKE_CURRENT_SOURCE_DIR}/sentium/api/v1/authz.proto)
cmake_path(SET authz_grpcxx_header ${CMAKE_CURRENT_BINARY_DIR}/sentium/api/v1/authz.grpcxx.pb.h)
cmake_path(SET authz_header ${CMAKE_CURRENT_BINARY_DIR}/sentium/api/v1/authz.pb.h)
cmake_path(SET authz_source ${CMAKE_CURRENT_BINARY_DIR}/sentium/api/v1/authz.pb.cc)

cmake_path(SET principals_proto  ${CMAKE_CURRENT_SOURCE_DIR}/sentium/api/v1/principals.proto)
cmake_path(SET principals_grpcxx_header ${CMAKE_CURRENT_BINARY_DIR}/sentium/api/v1/principals.grpcxx.pb.h)
cmake_path(SET principals_header ${CMAKE_CURRENT_BINARY_DIR}/sentium/api/v1/principals.pb.h)
cmake_path(SET principals_source ${CMAKE_CURRENT_BINARY_DIR}/sentium/api/v1/principals.pb.cc)

set(protos
	${authz_proto}
	${principals_proto}
)

set(headers
	${authz_header} ${authz_grpcxx_header}
	${principals_header} ${principals_grpcxx_header}
)

set(sources
	${authz_source}
	${principals_source}
)

add_custom_command(
	OUTPUT  ${headers} ${sources}
	DEPENDS ${protos}
	COMMAND ${Protobuf_PROTOC_EXECUTABLE}
	ARGS
		--proto_path=${CMAKE_CURRENT_SOURCE_DIR}
		--proto_path=${googleapis_SOURCE_DIR}
		--proto_path=${Protobuf_INCLUDE_DIR}
		--cpp_out=${CMAKE_CURRENT_BINARY_DIR}
		--grpcxx_out=${CMAKE_CURRENT_BINARY_DIR}
		--plugin=protoc-gen-grpcxx=$<TARGET_FILE:libgrpcxx::protoc-gen>
		${protos}
)

add_library(proto
	${sources}
)

target_include_directories(proto
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(proto
	googleapis
	libgrpcxx::grpcxx
	protobuf::libprotobuf
)

add_library(${PROJECT_NAME}::libproto ALIAS proto)
