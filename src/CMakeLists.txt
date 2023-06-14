add_subdirectory(datastore)
add_subdirectory(err)
add_subdirectory(service)

# datastore
add_library(libdatastore INTERFACE)
target_link_libraries(libdatastore
	INTERFACE datastore
)

target_include_directories(libdatastore
	INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_library(${PROJECT_NAME}::datastore ALIAS libdatastore)

# err
add_library(liberr INTERFACE)
target_link_libraries(liberr
	INTERFACE err
)

target_include_directories(liberr
	INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_library(${PROJECT_NAME}::err ALIAS liberr)

# service
add_library(libservice INTERFACE)
target_link_libraries(libservice
	INTERFACE service
)

target_include_directories(libservice
	INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_library(${PROJECT_NAME}::service ALIAS libservice)


# main
add_executable(${PROJECT_NAME}
	main.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	LINK_SEARCH_START_STATIC ON
	LINK_SEARCH_END_STATIC ON
)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		${PROJECT_NAME}::datastore
		${PROJECT_NAME}::service
		libviper::viper
)

