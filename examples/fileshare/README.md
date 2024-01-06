# File Share

### Generate pb files

```
protoc \
    --go_out=. \
    --go_opt=Msentium/api/v1/authz.proto=sentium/api/v1 \
    --go_opt=Msentium/api/v1/principals.proto=sentium/api/v1 \
    --go_opt=Msentium/api/v1/resources.proto=sentium/api/v1 \
    --go-grpc_opt=Msentium/api/v1/authz.proto=sentium/api/v1 \
    --go-grpc_opt=Msentium/api/v1/principals.proto=sentium/api/v1 \
    --go-grpc_opt=Msentium/api/v1/resources.proto=sentium/api/v1 \
    --go-grpc_out=. \
    --proto_path=/Users/ln/git/sentium/proto \
    --proto_path=. \
    --proto_path=/Users/ln/workbench/git/googleapis \
    sentium/api/v1/authz.proto sentium/api/v1/principals.proto sentium/api/v1/resources.proto
```