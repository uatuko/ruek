add_library(service
	grpc.cpp grpc.h
	mappers.cpp mappers.h
)

target_link_libraries(service
	PUBLIC
		${PROJECT_NAME}::libprotos
	PRIVATE
		${PROJECT_NAME}::datastore
)

