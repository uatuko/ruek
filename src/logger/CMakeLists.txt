add_library(logger
	logger.cpp logger.h
)

target_link_libraries(logger
	PRIVATE
		glaze::glaze
)
