FROM --platform=$BUILDPLATFORM debian:12-slim AS builder

ARG TARGETARCH

COPY . /tmp/source
WORKDIR /tmp

RUN apt-get update \
	&& \
	ruek_march=$(./source/bin/march.sh) \
	&& \
	if [ "$ruek_march" = "x86_64" ]; then ruek_march=x86-64; fi \
	&& \
	apt-get install -y --no-install-recommends \
		cmake ninja-build \
		binutils-${ruek_march}-linux-gnu g++ \
		protobuf-compiler libprotobuf-dev libprotoc-dev \
		libpq-dev

RUN ruek_march=$(./source/bin/march.sh) \
	&& \
	cmake -B build -G Ninja -S source/ \
		-DCMAKE_CXX_COMPILER_TARGET=${ruek_march}-linux-gnu \
		-DCMAKE_BUILD_TYPE=Release \
		-DRUEK_BUILD_TESTING=OFF

RUN cmake --build build/ --config Release


FROM debian:12-slim

RUN apt-get update \
	&& \
	apt-get install -y --no-install-recommends \
		libpq5 \
		libprotobuf32

COPY --from=builder /tmp/build/bin/ruek /opt/ruek/bin/

WORKDIR /opt/ruek
ENTRYPOINT [ "bin/ruek" ]
EXPOSE 8080
