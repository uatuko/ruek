<!-- omit in toc -->
# Relations (`ruek.api.v1.Relations`)

```proto
package ruek.api.v1;

service Relations {}
```

- [(rpc) Check (`ruek.api.v1.Relations.Check`)](#rpc-check-ruekapiv1relationscheck)
  - [Request message](#request-message)
  - [Response message](#response-message)
- [(rpc) Create (`ruek.api.v1.Relations.Create`)](#rpc-create-ruekapiv1relationscreate)
  - [Request message](#request-message-1)
  - [Response message](#response-message-1)
- [(rpc) Delete (`ruek.api.v1.Relations.Delete`)](#rpc-delete-ruekapiv1relationsdelete)
  - [Request message](#request-message-2)
  - [Response message](#response-message-2)
- [(rpc) Delete by Id (`ruek.api.v1.Relations.DeleteById`)](#rpc-delete-by-id-ruekapiv1relationsdeletebyid)
  - [Request message](#request-message-3)
  - [Response message](#response-message-3)
- [(rpc) ListLeft (`ruek.api.v1.Relations.ListLeft`)](#rpc-listleft-ruekapiv1relationslistleft)
  - [Request message](#request-message-4)
  - [Response message](#response-message-4)
- [(rpc) ListRight (`ruek.api.v1.Relations.ListRight`)](#rpc-listright-ruekapiv1relationslistright)
  - [Request message](#request-message-5)
  - [Response message](#response-message-5)
- [Messages](#messages)
  - [Entity](#entity)
  - [Tuple](#tuple)
  - [RelationsCheckRequest](#relationscheckrequest)
  - [RelationsCheckResponse](#relationscheckresponse)
  - [RelationsCreateRequest](#relationscreaterequest)
  - [RelationsCreateResponse](#relationscreateresponse)
  - [RelationsDeleteRequest](#relationsdeleterequest)
  - [RelationsDeleteResponse](#relationsdeleteresponse)
  - [RelationsDeleteByIdRequest](#relationsdeletebyidrequest)
  - [RelationsDeleteByIdResponse](#relationsdeletebyidresponse)
  - [RelationsListLeftRequest](#relationslistleftrequest)
  - [RelationsListLeftResponse](#relationslistleftresponse)
  - [RelationsListRightRequest](#relationslistrightrequest)
  - [RelationsListRightResponse](#relationslistrightresponse)
- [Appendix A. Strategies](#appendix-a-strategies)
  - [A.1. Lookup strategies](#a1-lookup-strategies)
  - [A.2. Optimization strategies](#a2-optimization-strategies)


## (rpc) Check (`ruek.api.v1.Relations.Check`)

Check if a relation exists.

```proto
rpc Check(RelationsCheckRequest) returns (RelationsCheckResponse);
```

### Request message

[`RelationsCheckRequest`](#relationscheckrequest)

### Response message

[`RelationsCheckResponse`](#relationscreateresponse)


## (rpc) Create (`ruek.api.v1.Relations.Create`)

Create a new relation.

```proto
rpc Create(RelationsCreateRequest) returns (RelationsCreateResponse);
```

### Request message

[`RelationsCreateRequest`](#relationscreaterequest)

### Response message

[`RelationsCreateResponse`](#relationscreateresponse)


## (rpc) Delete (`ruek.api.v1.Relations.Delete`)

Delete an existing relation.

```proto
rpc Delete(RelationsDeleteRequest) returns (RelationsDeleteResponse);
```

### Request message

[`RelationsDeleteRequest`](#relationsdeleterequest)

### Response message

[`RelationsDeleteResponse`](#relationsdeleteresponse)


## (rpc) Delete by Id (`ruek.api.v1.Relations.DeleteById`)

Delete an existing relation using the relation tuple id.

```proto
rpc Delete(RelationsDeleteByIdRequest) returns (RelationsDeleteByIdResponse);
```

### Request message

[`RelationsDeleteByIdRequest`](#relationsdeletebyidrequest)

### Response message

[`RelationsDeleteByIdResponse`](#relationsdeletebyidresponse)


## (rpc) ListLeft (`ruek.api.v1.Relations.ListLeft`)

List relations to the left of a relation.

```proto
rpc ListLeft(RelationsListLeftRequest) returns (RelationsListLeftResponse);
```

### Request message

[`RelationsListLeftRequest`](#relationslistleftrequest)

### Response message

[`RelationsListLeftResponse`](#relationslistleftresponse)


## (rpc) ListRight (`ruek.api.v1.Relations.ListRight`)

List relations to the right of a relation.

```proto
rpc ListRight(RelationsListRightRequest) returns (RelationsListRightResponse);
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
| strategy                       | (optional) `uint32`  | Lookup strategy to use (default `2`). See [lookup strategies](#a1-lookup-strategies). |
| cost_limit                     | (optional) `uint32`  | A value between `1` and `65535` to limit the lookup cost (default `1000`). |

### RelationsCheckResponse

| Field  | Type                         | Description |
| ------ | ---------------------------- | ----------- |
| found  | `bool`                       | Flag to indicate if a relation exists or could be derived using the lookup strategy. |
| cost   | `int32`                      | Lookup cost. A negative cost indicates the lookup cost exceeded the limit and the lookup _may_ have been abandoned without computing all possible derivations. |
| tuple  | (optional) [`Tuple`](#tuple) | Tuple containing relation data that matched the query. An empty tuple `id` indicates a computed tuple which isn't stored. |
| path   | [`[]Tuple`](#tuple)          | Path that derived the relation between entities when using the _graph_ (`4`) lookup strategy. |

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
| optimize   | (optional) `uint32` | Optimization strategy to use (default `4`). See [optimization strategies](#a2-optimization-strategies). |
| cost_limit | (optional) `uint32` | A value between `1` and `65535` to limit the cost of creating a new relation (default `1000`). |

### RelationsCreateResponse

| Field           | Type                | Description |
| --------------- | ------------------- | ----------- |
| tuple           | [`Tuple`](#tuple)   | Tuple containing the relation data. |
| cost            | `int32`             | Cost of creating the relation. A negative cost indicates only the relation was created but computing and storing derived relations was aborted. |
| computed_tuples | [`[]Tuple`](#tuple) | Computed and _maybe_ stored derived relation tuples. If the `cost` returned is negative, this _may_ contain a partial list. Any tuple with an empty id indicates it's only computed but not stored (i.e. dirty). |

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

### RelationsDeleteByIdRequest

| Field  | Type     | Description |
| ------ | -------- | ----------- |
| id     | `string` | Relation tuple id to delete. |

### RelationsDeleteByIdResponse

| Field | Type | Description |
| ----- | ---- | ----------- |

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

| Field            | Type                | Description |
| ---------------- | ------------------- | ----------- |
| tuples           | [`[]Tuple`](#tuple) | |
| pagination_token | (optional) `string` | |

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

| Field            | Type                | Description |
| ---------------- | ------------------- | ----------- |
| tuples           | [`[]Tuple`](#tuple) | |
| pagination_token | (optional) `string` | |


## Appendix A. Strategies

Refer to [ReBAC strategies](../../rebac.md#strategies) documentation for more information.

### A.1. Lookup strategies

Lookup strategies are used when checking

| Strategy     | Description |
| ------------ | ----------- |
| `2` (direct) | Only check if there's a direct relation exists between the entities. |
| `4` (graph)  | If a direct relation cannot be found between the entities, use a graph traversal algorithm to derive a relation. |
| `8` (set)    | Check if there's a direct relation exists between the entities and if not, use a set intersection algorithm to derive a relation between the entities. |

### A.2. Optimization strategies

| Strategy     | Description |
| ------------ | ----------- |
| `2` (direct) | Optimize for direct lookups. Best for lookup speeds but can produce large numbers of computed relations resulting in expensive write operations. |
| `4` (graph)  | Don't optimize. Lookups will need to use a graph traversal algorithm to derive relations. |
| `8` (set)    | Optimize by computing derived relations between principals, which the lookups can utilise in a set intersection algorithm. |
