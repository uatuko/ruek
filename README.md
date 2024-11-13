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


## 🔥 Features

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


## 📜 Documentation

You can find a bit more detailed documentation in [docs/](docs/README.md).


## 🚀 Quickstart (with `docker`)

### Create a docker network
```sh
docker network create ruek-net
```

### Run PostgreSQL
```sh
docker run --net=ruek-net --name=pg -e POSTGRES_PASSWORD=postgres -d postgres:16-alpine
```

### Setup PostgreSQL user and database for Ruek
```sh
echo "create user ruek with password 'ruek'; create database ruek owner ruek;" | docker run --rm -i --network=ruek-net -e PGPASSWORD=postgres postgres:16-alpine psql --host=pg --username=postgres
```
```sh
curl -sL https://raw.githubusercontent.com/uatuko/ruek/refs/heads/main/db/schema.sql | docker run --rm -i --network=ruek-net -e PGPASSWORD=ruek postgres:16-alpine psql --host=pg --username=ruek --dbname=ruek
```

### Run Ruek
```sh
docker run --network=ruek-net --name=ruek -e PGHOST=pg -e PGUSER=ruek -e PGPASSWORD=ruek -p 8080:8080 -d uatuko/ruek:latest
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
