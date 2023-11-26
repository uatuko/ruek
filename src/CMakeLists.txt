add_subdirectory(svc)

# svc
add_library(libsvc INTERFACE)
target_link_libraries(libsvc
	INTERFACE svc
)

target_include_directories(libsvc
	INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_library(${PROJECT_NAME}::svc ALIAS libsvc)


# main
add_executable(${PROJECT_NAME}
	main.cpp
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		${PROJECT_NAME}::svc
)
