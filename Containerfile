FROM debian:12-slim as builder

RUN apt-get update \
	&& \
	apt-get install -y --no-install-recommends \
		cmake ninja-build \
		clang libclang-rt-dev \
		protobuf-compiler libprotobuf-dev libprotoc-dev \
		libpq-dev

COPY . /tmp/source
WORKDIR /tmp

RUN cmake -B build -G Ninja -S source/ \
	-DCMAKE_BUILD_TYPE=Release \
	-DSENTIUM_BUILD_TESTING=OFF

RUN cmake --build build/ --config Release


FROM debian:12-slim

RUN apt-get update \
	&& \
	apt-get install -y --no-install-recommends \
		libpq5 \
		libprotobuf32

COPY --from=builder /tmp/build/bin/sentium /opt/sentium/bin/

WORKDIR /opt/sentium
ENTRYPOINT [ "bin/sentium" ]
EXPOSE 8080
