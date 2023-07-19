add_subdirectory(datastore)
add_subdirectory(err)
add_subdirectory(events)
add_subdirectory(logger)
add_subdirectory(svc)

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

# events
add_library(libevents INTERFACE)
target_link_libraries(libevents
	INTERFACE events
)

target_include_directories(libevents
	INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_library(${PROJECT_NAME}::events ALIAS libevents)

# logger
add_library(liblogger INTERFACE)
target_link_libraries(liblogger
	INTERFACE logger
)

target_include_directories(liblogger
	INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_library(${PROJECT_NAME}::logger ALIAS liblogger)

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
		${PROJECT_NAME}::svc
		libviper::viper
)
