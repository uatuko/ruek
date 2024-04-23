# 🔐 Sentium

[![license](https://img.shields.io/github/license/uatuko/sentium)](https://raw.githubusercontent.com/uatuko/sentium/main/LICENSE)
[![codecov](https://codecov.io/gh/uatuko/sentium/graph/badge.svg?token=KR9MkDkk8s)](https://codecov.io/gh/uatuko/sentium)
[![discussions](https://img.shields.io/github/discussions/uatuko/sentium)](https://github.com/uatuko/sentium/discussions)
[![release](https://img.shields.io/github/v/release/uatuko/sentium)](https://github.com/uatuko/sentium/releases)

Lightning fast, global scale authorization service without the overhead of yet another DSL[^5].

## What is Sentium?

Sentium is an authorization service for securing your applications and services using zero trust[^1]
fine-grained authorization (FGA).

We designed Sentium to be as powerful and scalable as [Zanzibar — Google’s Consistent, Global Authorization System](https://research.google/pubs/zanzibar-googles-consistent-global-authorization-system/)
yet simple enough to start using without the overhead of having to learn a new DSL to define authorization models or policies.

### Why Sentium?

There are other open-source (and commercial) authorization services, some are inspired by Google Zanzibar
while others tend to offer policy-as-code solutions. But almost all of these solutions require learning
a new DSL to create authorization models or define policies, which adds unnecessary complexities.

Using an authorization service shouldn't come with a requirement to be an expert in building and maintaining
authorization models or policies. It should be as easy as using an API.

Sentium lean on well known API design principals to provide an authorization service that's easy to
integrate, quick to master and flexible enough to handle complex requirements.


## Features

* Schema-less fine-grained authorization (FGA)
* Zero-trust, least privilege architecture (ZTA)
* Predictable constant time authorization checks (O(1))
* Strongly consistent with no cache
* Cloud native at global scale[^2]
* ABAC, RBAC & ReBAC[^4]
* Multi-tenancy support, if you need it
* Not just authorization checks, list users, resources a user can access and users with access to a resource
* First class treatment for listing endpoints with pagination and limits to handle large datasets
* Built using the fastest gRPC server implementation[^3]


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
    "entity_type": "documents",
    "entity_id": "65bd28aaa076ee8c8463cff8"
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
    "entity_type": "documents",
    "entity_id": "65bd28aaa076ee8c8463cff8"
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

* Thanks to [@kw510](https://github.com/kw510), [@neculalaura](https://github.com/neculalaura) and [@td0m](https://github.com/td0m)
for their contributions on the `gatekeeper` branch.

[^1]: [Zero trust architecture (ZTA)](https://en.wikipedia.org/wiki/Zero_trust_security_model)
[^2]: Scalability depends on underlying PostgreSQL protocol compatible database scalability.
[^3]: [gRPCxx](https://github.com/uatuko/grpcxx) is benchmarked to be the fastest in February 2024.
[^4]: [RFC #72](https://github.com/uatuko/sentium/discussions/72)
[^5]: [Domain-Specific Language](https://en.wikipedia.org/wiki/Domain-specific_language)
