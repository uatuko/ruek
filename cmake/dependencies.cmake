include(FetchContent)

# protobuf
find_package(Protobuf REQUIRED)

# grpcxx
FetchContent_Declare(grpcxx
	URL      https://github.com/uatuko/grpcxx/archive/0cf174e4b17d4a3b891dc8d9b95d6432730bbe2f.tar.gz
	URL_HASH SHA256=02cac9210360ed4cf52d6262b35304da1cbba49bd15f402dbf491836dfa47a3c
)
FetchContent_MakeAvailable(grpcxx)

# googleapis
FetchContent_Declare(googleapis
	URL      https://github.com/googleapis/googleapis/archive/0e3b813b0d0da539eacbe86b8716feeed00943c5.tar.gz
	URL_HASH SHA256=44f3b9c73a5df760c4fad3cf0c5cc54732b881f00708308f7635ff75a13dcaa5
)
FetchContent_MakeAvailable(googleapis)
