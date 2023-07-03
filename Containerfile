FROM debian:bookworm-slim as devtools

RUN apt-get update && apt-get install -y --no-install-recommends clang libclang-rt-dev
RUN apt-get update && apt-get install -y --no-install-recommends cmake ninja-build
RUN apt-get update && apt-get install -y --no-install-recommends libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc
RUN apt-get update && apt-get install -y --no-install-recommends libpq-dev postgresql-client

FROM devtools AS build

WORKDIR /home/build

COPY CMakeLists.txt .
COPY src ./src
COPY conf ./conf
COPY cmake ./cmake
COPY proto/ ./proto

RUN cmake -B .build -G Ninja \ 
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      && cmake --build .build --config Release

FROM debian:bookworm-slim AS deploy

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends libpq5 libgrpc++1.51

COPY conf ./conf
COPY --from=build /home/build/.build/bin/gatekeeper ./gatekeeper

EXPOSE 8080
CMD [ "/app/gatekeeper" ]
