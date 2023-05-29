add_library(service
	grpc.cpp grpc.h
)

target_link_libraries(service
	PUBLIC
		${PROJECT_NAME}::libprotos
)

