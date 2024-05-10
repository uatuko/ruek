add_executable(${PROJECT_NAME}_bench
	main.cpp
	relations_test.cpp
)

target_link_libraries(${PROJECT_NAME}_bench
	PRIVATE
		${PROJECT_NAME}::db
		${PROJECT_NAME}::svc
		benchmark::benchmark
		libxid::xid
)
