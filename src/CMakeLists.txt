add_subdirectory(db)
add_subdirectory(err)
add_subdirectory(svc)

# db
add_library(libdb INTERFACE)
target_link_libraries(libdb
	INTERFACE db
)

target_include_directories(libdb
	INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_library(${PROJECT_NAME}::db ALIAS libdb)

# err
add_library(liberr INTERFACE)
target_link_libraries(liberr
	INTERFACE err
)

target_include_directories(liberr
	INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_library(${PROJECT_NAME}::err ALIAS liberr)

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
		${PROJECT_NAME}::db
		${PROJECT_NAME}::svc
)
