# Principals (`ruek.api.v1.Principals`)

```proto
package ruek.api.v1;

service Principals {}
```

## [RPC] Create (`ruek.api.v1.Principals.Create`)

Create a new principal.

```proto
rpc Create(PrincipalsCreateRequest) returns (Principal) {
  option (google.api.http) = {
    post : "/v1/principals"
    body : "*"
  };
}
```

### Request message

[`PrincipalsCreateRequest`](#PrincipalsCreateRequest)


### Response message

[`Principal`](#Principal)


### Example

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/ruek/api/v1/principals.proto \
  -plaintext \
  localhost:8080 ruek.api.v1.Principals/Create

{
  "id": "cn7qtdu56a1cqrj8kur0"
}
```


## Messages

### Principal

| Field   | Type                | Description |
| ------- | ------------------- | ----------- |
| id      | `string`            | |
| attrs   | (optional) [google.protobuf.Struct](https://protobuf.dev/reference/protobuf/google.protobuf/#struct) | |
| segment | (optional) `string` | |

### PrincipalsCreateRequest

| Field   | Type                | Description |
| ------- | ------------------- | ----------- |
| id      | (optional) `string` | |
| attrs   | (optional) [google.protobuf.Struct](https://protobuf.dev/reference/protobuf/google.protobuf/#struct) | |
| segment | (optional) `string` | |
