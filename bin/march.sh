#!/usr/bin/env sh

: ${ruek_march=unknown}

if [ -z ${TARGETARCH} ]; then
	TARGETARCH=$(uname -m)
fi

case ${TARGETARCH} in
	amd64 | x86_64)
		ruek_march=x86_64
		;;
	aarch64 | arm64)
		ruek_march=aarch64
		;;
esac

echo ${ruek_march}
