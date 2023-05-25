add_library(datastore
	datastore.h
	pg.cpp
	pg.h
)

target_link_libraries(datastore
	PUBLIC
		pqxx
)

