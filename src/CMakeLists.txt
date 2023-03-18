# main
add_executable(${PROJECT_NAME}
	main.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	LINK_SEARCH_START_STATIC ON
	LINK_SEARCH_END_STATIC ON
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_libraries(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()
