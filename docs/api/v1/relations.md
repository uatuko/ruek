# Relations (`ruek.api.v1.Relations`)

```proto
package ruek.api.v1;

service Relations {}
```

## [RPC] Check (`ruek.api.v1.Relations.Check`)

Check if a relation exists.

```proto
rpc Check(RelationsCheckRequest) returns (RelationsCheckResponse) {
  option (google.api.http) = {
    post : "/v1/relations:check"
    body : "*"
  };
}
```

### Request message

[`RelationsCheckRequest`](#relationscheckrequest)

### Response message

[`RelationsCheckResponse`](#relationscreateresponse)


## [RPC] Create (`ruek.api.v1.Relations.Create`)

Create a new relation.

```proto
rpc Create(RelationsCreateRequest) returns (RelationsCreateResponse) {
  option (google.api.http) = {
    post : "/v1/relations"
    body : "*"
  };
}
```

### Request message

[`RelationsCreateRequest`](#relationscreaterequest)

### Response message

[`RelationsCreateResponse`](#relationscreateresponse)


## [RPC] Delete (`ruek.api.v1.Relations.Delete`)

Delete an existing relation.

```proto
rpc Delete(RelationsDeleteRequest) returns (RelationsDeleteResponse) {
  option (google.api.http) = {
    post : "/v1/relations:delete"
    body : "*"
  };
}
```

### Request message

[`RelationsDeleteRequest`](#relationsdeleterequest)

### Response message

[`RelationsDeleteResponse`](#relationsdeleteresponse)


## [RPC] ListLeft (`ruek.api.v1.Relations.ListLeft`)

List relations to the left of a relation.

```proto
rpc ListLeft(RelationsListLeftRequest) returns (RelationsListLeftResponse) {
  option (google.api.http) = {
    get : "/v1/relations:left?_limit={pagination_limit}&_start={pagination_token}"
  };
}
```

### Request message

[`RelationsListLeftRequest`](#relationslistleftrequest)

### Response message

[`RelationsListLeftResponse`](#relationslistleftresponse)


## [RPC] ListRight (`ruek.api.v1.Relations.ListRight`)

List relations to the right of a relation.

```proto
rpc ListRight(RelationsListRightRequest) returns (RelationsListRightResponse) {
  option (google.api.http) = {
    get : "/v1/relations:right?_limit={pagination_limit}&_start={pagination_token}"
  };
}
```

### Request message

[`RelationsListRightRequest`](#relationslistrightrequest)

### Response message

[`RelationsListRightResponse`](#relationslistrightresponse)


## Messages

### Entity

| Field  | Type     | Description |
| ------ | -------- | ----------- |
| id     | `string` | |
| type   | `string` | |

### Tuple

| Field                          | Type                 | Description |
| ------------------------------ | -------------------- | ----------- |
| space_id                       | `string`             | |
| id                             | `string`             | |
| `left`                         | (oneof)              | |
| [ `left` ] left_entity         |  [`Entity`](#entity) | |
| [ `left` ] left_principal_id   | `string`             | |
| relation                       | `string`             | |
| `right`                        | (oneof)              | |
| [ `right` ] right_entity       |  [`Entity`](#entity) | |
| [ `right` ] right_principal_id |  `string`            | |
| strand                         | (optional) `string`  | |
| attrs        | (optional) [`google.protobuf.Struct`](https://protobuf.dev/reference/protobuf/google.protobuf/#struct) | |
| ref_id_left  | (optional) `string` | |
| ref_id_right | (optional) `string` | |

### RelationsCheckRequest

| Field                          | Type                 | Description |
| ------------------------------ | -------------------- | ----------- |
| `left`                         | (oneof)              | |
| [ `left` ] left_entity         |  [`Entity`](#entity) | |
| [ `left` ] left_principal_id   | `string`             | |
| relation                       | `string`             | |
| `right`                        | (oneof)              | |
| [ `right` ] right_entity       |  [`Entity`](#entity) | |
| [ `right` ] right_principal_id |  `string`            | |
| strategy                       | (optional) `uint32`  | |
| cost_limit                     | (optional) `uint32`  | |

### RelationsCheckResponse

| Field  | Type                         | Description |
| ------ | ---------------------------- | ----------- |
| found  | `bool`                       | |
| cost   | `int32`                      | |
| tuple  | (optional) [`Tuple`](#tuple) | |
| path   | `[]`[`Tuple`](#tuple)        | |

### RelationsCreateRequest

| Field                          | Type                 | Description |
| ------------------------------ | -------------------- | ----------- |
| `left`                         | (oneof)              | |
| [ `left` ] left_entity         |  [`Entity`](#entity) | |
| [ `left` ] left_principal_id   | `string`             | |
| relation                       | `string`             | |
| `right`                        | (oneof)              | |
| [ `right` ] right_entity       |  [`Entity`](#entity) | |
| [ `right` ] right_principal_id |  `string`            | |
| strand                         | (optional) `string`  | |
| attrs      | (optional) [`google.protobuf.Struct`](https://protobuf.dev/reference/protobuf/google.protobuf/#struct) | |
| optimize   | (optional) `uint32` | |
| cost_limit | (optional) `uint32` | |

### RelationsCreateResponse

| Field           | Type                  | Description |
| --------------- | --------------------- | ----------- |
| tuple           | [`Tuple`](#tuple)     | |
| cost            | `int32`               | |
| computed_tuples | `[]`[`Tuple`](#tuple) | |

### RelationsDeleteRequest

| Field                          | Type                 | Description |
| ------------------------------ | -------------------- | ----------- |
| `left`                         | (oneof)              | |
| [ `left` ] left_entity         |  [`Entity`](#entity) | |
| [ `left` ] left_principal_id   | `string`             | |
| relation                       | `string`             | |
| `right`                        | (oneof)              | |
| [ `right` ] right_entity       |  [`Entity`](#entity) | |
| [ `right` ] right_principal_id |  `string`            | |
| strand                         | (optional) `string`  | |

### RelationsDeleteResponse

| Field  | Type | Description |
| ------ | ---- | ----------- |

### RelationsListLeftRequest

| Field                          | Type                 | Description |
| ------------------------------ | -------------------- | ----------- |
| `right`                        | (oneof)              | |
| [ `right` ] right_entity       |  [`Entity`](#entity) | |
| [ `right` ] right_principal_id | `string`             | |
| relation                       | (optional) `string`  | |
| pagination_limit               | (optional) `uint32`  | |
| pagination_token               | (optional) `string`  | |

### RelationsListLeftResponse

| Field            | Type                  | Description |
| ---------------- | --------------------  | ----------- |
| tuples           | `[]`[`Tuple`](#tuple) | |
| pagination_token | (optional) `string`   | |

### RelationsListRightRequest

| Field                        | Type                 | Description |
| ---------------------------- | -------------------- | ----------- |
| `left`                       | (oneof)              | |
| [ `left` ] left_entity       |  [`Entity`](#entity) | |
| [ `left` ] left_principal_id | `string`             | |
| relation                     | (optional) `string`  | |
| pagination_limit             | (optional) `uint32`  | |
| pagination_token             | (optional) `string`  | |

### RelationsListRightResponse

| Field            | Type                  | Description |
| ---------------- | --------------------- | ----------- |
| tuples           | `[]`[`Tuple`](#tuple) | |
| pagination_token | (optional) `string`   | |
