FROM --platform=linux/amd64 debian:bookworm-slim as devtools

RUN apt-get update && apt-get install -y --no-install-recommends \
      clang libclang-rt-dev \
      cmake ninja-build \
      libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
      libpq-dev postgresql-client

FROM --platform=linux/amd64 devtools AS build

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

FROM --platform=linux/amd64 debian:bookworm-slim AS deploy

WORKDIR /opt/gatekeeper

RUN apt-get update && apt-get install -y --no-install-recommends libpq5 libgrpc++1.51

COPY conf ./conf

COPY --from=build /home/build/.build/bin ./bin

EXPOSE 7000
CMD [ "./bin/gatekeeper" ]
