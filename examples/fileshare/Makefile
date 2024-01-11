sourcedir = .

lint:
	gofmt -s -d -e $(sourcedir)

lint\:fix:
	gofmt -s -l -w $(sourcedir)

run:
	go run main.go

pb: protoc-gen-go protoc-gen-go-grpc
	protoc \
		--go_out=./pkg/pb \
		--go_opt=Msentium/api/v1/authz.proto=sentium/api/v1 \
		--go_opt=Msentium/api/v1/principals.proto=sentium/api/v1 \
		--go_opt=Msentium/api/v1/resources.proto=sentium/api/v1 \
		--go-grpc_opt=Msentium/api/v1/authz.proto=sentium/api/v1 \
		--go-grpc_opt=Msentium/api/v1/principals.proto=sentium/api/v1 \
		--go-grpc_opt=Msentium/api/v1/resources.proto=sentium/api/v1 \
		--go-grpc_out=./pkg/pb \
		--proto_path=../../proto \
		--proto_path=../../.build/_deps/googleapis-src \
		sentium/api/v1/authz.proto \
		sentium/api/v1/principals.proto \
		sentium/api/v1/resources.proto

protoc-gen-go:
ifeq (, $(shell which protoc-gen-go))
	go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
endif

protoc-gen-go-grpc:
ifeq (, $(shell which protoc-gen-go-grpc))
	go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
endif

test:
	go test -race ./...
