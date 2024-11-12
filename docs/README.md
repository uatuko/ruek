# 📜 Documentation

## Table of contents

* [Getting started](#getting-started)
* [API Documentation](api/README.md)
* [ReBAC](rebac.md)


## Getting started

### Prerequisites

Ruek require [PostgreSQL](https://www.postgresql.org) (or [PostgreSQL protocol](https://www.postgresql.org/docs/current/protocol.html) compatible server) to run.

<details>
<summary>Example - setting up PostgreSQL in docker</summary>

```sh
# create a docker network to connect postgres and ruek containers
docker network create ruek-net

# run postgres in the 'ruek-net' network
docker run --net=ruek-net --name=pg -e POSTGRES_PASSWORD=postgres -d postgres:16-alpine

# wait until postgres starts up
while ! $(docker run --rm --network=ruek-net postgres:16-alpine pg_isready --host=pg | grep -q 'accepting connections'); do sleep 1; done

# setup postgres user and db for ruek
echo "create user ruek with password 'ruek'; create database ruek owner ruek;" | docker run --rm -i --network=ruek-net -e PGPASSWORD=postgres postgres:16-alpine psql --host=pg --username=postgres

# initialize ruek db
curl -sL https://raw.githubusercontent.com/uatuko/ruek/refs/heads/main/db/schema.sql | docker run --rm -i --network=ruek-net -e PGPASSWORD=ruek postgres:16-alpine psql --host=pg --username=ruek --dbname=ruek
```
</details>

### Using pre-built containers

The quickest way to run Ruek is by using pre-built containers. There are `linux/amd64` and `linux/arm64` containers published to [`docker.io/uatuko/ruek`](https://hub.docker.com/r/uatuko/ruek) and [`ghcr.io/uatuko/ruek`](https://github.com/uatuko/ruek/pkgs/container/ruek).

> [!IMPORTANT]
> In the following example, Ruek is connecting to a PostgreSQL server running on host `pg` using the username `ruek` and password `ruek`. If you are running PostgreSQL in docker, you probably will need to specify the `--network` option to connect to a container network (e.g. `--network=ruek-net`)

e.g.
```sh
docker run --name=ruek -e PGHOST=pg -e PGUSER=ruek -e PGPASSWORD=ruek -p 8080:8080 -d uatuko/ruek:latest
```

### Compiling from source

#### Prerequisites

* [CMake](https://cmake.org) (>= 3.23)
* [Protobuf](https://protobuf.dev) (>= 3.15)
* [libpq](https://www.postgresql.org/docs/current/libpq.html)

#### Build with CMake

> [!NOTE]
> The following should result in a Ruek executable at  `.build/bin/ruek`.

```sh
# generate build files
cmake -B .build \
  -DCMAKE_BUILD_TYPE=Release \
  -DPostgreSQL_ADDITIONAL_VERSIONS=16 \
  -DRUEK_ENABLE_COVERAGE=OFF \
  -DRUEK_BUILD_TESTING=OFF

# compile
cmake --build .build --target ruek
```


### Setting-up PostgreSQL

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
