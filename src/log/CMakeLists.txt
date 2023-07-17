add_library(log
	log.cpp log.h
)

target_link_libraries(log
	PRIVATE
		glaze::glaze
)
