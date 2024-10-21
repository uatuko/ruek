# 🔐 Ruek

[![license](https://img.shields.io/github/license/uatuko/ruek)](https://raw.githubusercontent.com/uatuko/ruek/main/LICENSE)
[![codecov](https://codecov.io/gh/uatuko/ruek/graph/badge.svg?token=KR9MkDkk8s)](https://codecov.io/gh/uatuko/ruek)
[![discussions](https://img.shields.io/github/discussions/uatuko/ruek)](https://github.com/uatuko/ruek/discussions)
[![release](https://img.shields.io/github/v/release/uatuko/ruek)](https://github.com/uatuko/ruek/releases)

Lightning fast, global scale authorization service without the overhead of a yet another DSL[^1].

## What is Ruek?

Ruek is an authorization service for securing your applications and services using zero trust[^2]
fine-grained authorization (FGA).

We designed Ruek to be as powerful and scalable as [Zanzibar — Google’s Consistent, Global Authorization System](https://research.google/pubs/zanzibar-googles-consistent-global-authorization-system/)
yet simple enough to start using without the overhead of having to learn a new DSL to define authorization models or policies.

### Why Ruek?

There are other open-source (and commercial) authorization services, some are inspired by Google Zanzibar
while others tend to offer policy-as-code solutions. But almost all of these solutions require learning
a new DSL to create authorization models or define policies, which adds unnecessary complexities.

Using an authorization service shouldn't come with a requirement to be an expert in building and maintaining
authorization models or policies. It should be as easy as using an API.

Ruek lean on well known API design principals to provide an authorization service that's easy to
integrate, quick to master and flexible enough to handle complex requirements.


## Features

* ABAC, RBAC & ReBAC
* Schema-less fine-grained authorization (FGA)
* Zero-trust, least privilege architecture (ZTA)
* Predictable constant time authorization checks (**O(1)**)[^3]
* Strongly consistent with no cache
* Cloud native at global scale[^4]
* Multi-tenancy support, if you need it
* Not just authorization checks, list users, entities a user can access and users with access to an entity
* First class treatment for listing endpoints with pagination and limits to handle large datasets
* Built using the fastest gRPC server implementation[^5]


## Documentation

You can find a bit more detailed documentation in [docs/](docs/README.md).


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
  -Druek_ENABLE_COVERAGE=OFF
```

```
❯ cmake --build .build --target ruek
```

### Setting-up

```
❯ psql --dbname=postgres
psql (16.1)
Type "help" for help.

postgres=# create user ruek;
CREATE ROLE
postgres=# create database ruek owner ruek;
CREATE DATABASE
```

```
❯ psql --username=ruek --dbname=ruek < db/schema.sql
```

### Running

```
❯ PGDATABASE=ruek PGUSER=ruek ./.build/bin/ruek
Listening on [127.0.0.1:8080] ...
```


## Usage

### Creating a user

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

### Granting access

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/ruek/api/v1/authz.proto \
  -plaintext \
  -d '{
    "principal_id": "cn7qtdu56a1cqrj8kur0",
    "entity_type": "documents",
    "entity_id": "65bd28aaa076ee8c8463cff8"
  }' \
  localhost:8080 ruek.api.v1.Authz/Grant

{}
```

### Checking access

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/ruek/api/v1/authz.proto \
  -plaintext \
  -d '{
    "principal_id": "cn7qtdu56a1cqrj8kur0",
    "entity_type": "documents",
    "entity_id": "65bd28aaa076ee8c8463cff8"
  }' \
  localhost:8080 ruek.api.v1.Authz/Check

{
  "ok": true
}
```

### Listing users

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/ruek/api/v1/principals.proto \
  -plaintext \
  localhost:8080 ruek.api.v1.Principals/List

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

### Listing entities a user can access

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/ruek/api/v1/entities.proto \
  -plaintext \
  -d '{
    "principal_id": "cn7qtdu56a1cqrj8kur0",
    "entity_type": "documents"
  }' \
  localhost:8080 ruek.api.v1.Entities/List

{
  "entities": [
    {
      "id": "65bd28aaa076ee8c8463cff8",
      "type": "documents"
    }
  ]
}
```

### Listing users that has access to an entity

```
❯ grpcurl \
  -import-path proto \
  -import-path ./.build/_deps/googleapis-src \
  -proto proto/ruek/api/v1/entities.proto \
  -plaintext \
  -d '{
    "entity_type": "documents",
    "entity_id": "65bd28aaa076ee8c8463cff8"
  }' \
  localhost:8080 ruek.api.v1.Entities/ListPrincipals

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
* [benchmark](https://github.com/google/benchmark) - For benchmarks.
* [grpcxx](https://github.com/uatuko/grpcxx) - For the gRPC server.
* [libpqxx](https://github.com/jtv/libpqxx) - For PostgreSQL connections.
* [libxid](https://github.com/uatuko/libxid) - For globally unique IDs.


[^1]: [Domain-Specific Language](https://en.wikipedia.org/wiki/Domain-specific_language)
[^2]: [Zero trust architecture (ZTA)](https://en.wikipedia.org/wiki/Zero_trust_security_model)
[^3]: Authorization check using ReBAC `set` (**O(1+n+m)**) and `graph` (**O(1+v+e)**) strategies are not constant time.
[^4]: Scalability depends on underlying PostgreSQL protocol compatible database scalability.
[^5]: [gRPCxx](https://github.com/uatuko/grpcxx) is benchmarked to be the fastest in February 2024.
