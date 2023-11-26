add_library(svc)
target_sources(svc
	PRIVATE
		principals.cpp
	PUBLIC
		FILE_SET headers TYPE HEADERS
		FILES
			principals.h
)

target_link_libraries(svc
	PUBLIC
		${PROJECT_NAME}::libproto
)
