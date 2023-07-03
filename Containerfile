FROM debian:bookworm-slim as builder

WORKDIR /app

COPY CMakeLists.txt .

COPY src ./src
COPY conf ./conf
COPY cmake ./cmake
COPY proto/ ./proto

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      clang libclang-rt-dev \
      cmake ninja-build \
      libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
      libpq-dev postgresql-client

RUN cmake -B .build -G Ninja \ 
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_BUILD_TYPE=Release

RUN cmake --build .build --config Release

EXPOSE 8080
CMD [ "/app/.build/bin/gatekeeper" ]
