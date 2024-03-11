# 🔐 Sentium

[![license](https://img.shields.io/badge/license-MIT-green)](https://raw.githubusercontent.com/uatuko/sentium/main/LICENSE)
[![codecov](https://codecov.io/gh/uatuko/sentium/graph/badge.svg?token=KR9MkDkk8s)](https://codecov.io/gh/uatuko/sentium)

Lightning fast, global scale authorisation service without the overhead of (yet another) modeling language.

## Features

* Schema-less fine-grained access control (FGA)
* Zero-trust, least privilege architecture (ZTA)
* Predictable constant time authorisation checks (O(1))
* Strongly consistent with no cache
* Cloud native at global scale[^1]
* ABAC, RBAC & ReBAC (with constraints)
* Not just authorisation checks, list users, resources a user can access and users with access to a resource
* First class treatment for listing endpoints with pagination and limits to handle large datasets
* Built using the fastest gRPC server implementation[^2]


## Getting started

### Prerequisites

* [CMake](https://cmake.org) (>= 3.23)
* [Protobuf](https://protobuf.dev) (>= 3.15)
* [libpq](https://www.postgresql.org/docs/current/libpq.html)
* [PostgreSQL](https://www.postgresql.org) (or [PostgreSQL protocol](https://www.postgresql.org/docs/current/protocol.html) compatible server)

### Compiling

```
❯ cmake -B .build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DPostgreSQL_ADDITIONAL_VERSIONS=16 \
  -DSENTIUM_ENABLE_COVERAGE=OFF
```

```
❯ cmake --build .build --target sentium
```

### Setting-up

```
❯ psql --dbname=postgres
psql (16.1)
Type "help" for help.

postgres=# create user sentium;
CREATE ROLE
postgres=# create database sentium owner sentium;
CREATE DATABASE
```

```
❯ psql --username=sentium --dbname=sentium < db/schema.sql
```

### Running

```
❯ PGDATABASE=sentium PGUSER=sentium ./.build/bin/sentium
Listening on [127.0.0.1:8080] ...
```


## Usage

### Creating a user

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/sentium/api/v1/principals.proto \
  -plaintext \
  localhost:8080 sentium.api.v1.Principals/Create

{
  "id": "cn7qtdu56a1cqrj8kur0"
}
```

### Granting access

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/sentium/api/v1/authz.proto \
  -plaintext \
  -d '{
    "principal_id": "cn7qtdu56a1cqrj8kur0",
    "resource_type": "documents",
    "resource_id": "65bd28aaa076ee8c8463cff8"
  }' \
  localhost:8080 sentium.api.v1.Authz/Grant

{}
```

### Checking access

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/sentium/api/v1/authz.proto \
  -plaintext \
  -d '{
    "principal_id": "cn7qtdu56a1cqrj8kur0",
    "resource_type": "documents",
    "resource_id": "65bd28aaa076ee8c8463cff8"
  }' \
  localhost:8080 sentium.api.v1.Authz/Check

{
  "ok": true
}
```

### Listing users

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/sentium/api/v1/principals.proto \
  -plaintext \
  localhost:8080 sentium.api.v1.Principals/List

{
  "principals": [
    {
      "id": "cn7qtim56a1cqrj8kurg"
    },
    {
      "id": "cn7qtdu56a1cqrj8kur0"
    }
  ]
}
```

### Listing resources a user can access

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/sentium/api/v1/resources.proto \
  -plaintext \
  -d '{
    "principal_id": "cn7qtdu56a1cqrj8kur0",
    "resource_type": "documents"
  }' \
  localhost:8080 sentium.api.v1.Resources/List

{
  "resources": [
    {
      "id": "65bd28aaa076ee8c8463cff8",
      "type": "documents"
    }
  ]
}
```

### Listing users that has access to a resource

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/sentium/api/v1/resources.proto \
  -plaintext \
  -d '{
    "resource_type": "documents",
    "resource_id": "65bd28aaa076ee8c8463cff8"
  }' \
  localhost:8080 sentium.api.v1.Resources/ListPrincipals

{
  "principals": [
    {
      "id": "cn7qtdu56a1cqrj8kur0"
    }
  ]
}
```


## Built with

* [fmt](https://github.com/fmtlib/fmt) - For string formatting.
* [googleapis](https://github.com/googleapis/googleapis) - For annotations to help with gRPC/JSON transcoding.
* [googletest](https://github.com/google/googletest) - For tests.
* [grpcxx](https://github.com/uatuko/grpcxx) - For the gRPC server.
* [libpqxx](https://github.com/jtv/libpqxx) - For PostgreSQL connections.
* [libxid](https://github.com/uatuko/libxid) - For globally unique IDs.


## Acknowledgments

* Thanks to [@kw510](https://github.com/kw510), [@neculalaura](https://github.com/neculalaura) and [@td0m](https://github.com/td0m) for their contributions on the `gatekeeper` branch.

[^1]: Scalability depends on underlying PostgreSQL protocol compatible database scalability.
[^2]: [gRPCxx](https://github.com/uatuko/grpcxx) is benchmarked to be the fastest in February 2024.
