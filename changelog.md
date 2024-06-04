# Changelog

## v0.3.1 - 4th June, 2024

### 🔦 Spotlight
* Significantly reduce the lookup costs for checking relations when using _graph_ strategy (https://github.com/uatuko/sentium/pull/103)


### What's Changed
* Traverse relations graph right to left when checking relations by @uatuko in https://github.com/uatuko/sentium/pull/103

**Full Changelog**: https://github.com/uatuko/sentium/compare/v0.3.0...v0.3.1


## v0.3.0 - 1st June, 2024

### 🔦 Spotlight
* Full ReBAC implementation with multiple optimisation strategies (https://github.com/uatuko/sentium/issues/73)

### ⚠️ Breaking changes
* **DB**: Unused `_rid` column is removed from `tuples` table and replaced by new `_rid_l` and `_rid_r` columns (https://github.com/uatuko/sentium/pull/98)
* **gRPC**: `cost` fields in _Relations_ service response messages have been changed from `uint32` to `int32` (https://github.com/uatuko/sentium/pull/98)

### What's Changed
* Bump golang.org/x/net from 0.19.0 to 0.23.0 in /examples/fileshare by @dependabot in https://github.com/uatuko/sentium/pull/87
* ReBAC: Optimise when creating relations by @uatuko in https://github.com/uatuko/sentium/pull/98
* ReBAC: Optimisation strategies by @uatuko in https://github.com/uatuko/sentium/pull/99
* ReBAC: `set` strategy for checking relations by @uatuko in https://github.com/uatuko/sentium/pull/100
* ReBAC: `graph` strategy for checking relations by @uatuko in https://github.com/uatuko/sentium/pull/101
* Update docs by @uatuko in https://github.com/uatuko/sentium/pull/102

**Full Changelog**: https://github.com/uatuko/sentium/compare/v0.2.0...v0.3.0


## v0.2.0 - 27th April, 2024

### 🔦 Spotlight
* ReBAC implementation without computed relations or optimisations (using Relation Tuples proposal from RFC https://github.com/uatuko/sentium/discussions/72)
* Multi-platform containers (https://github.com/uatuko/sentium/pull/93, https://github.com/uatuko/sentium/pull/95)

### ⚠️ Breaking changes
* Remove Resources gRPC service and introduce Entities gRPC service (https://github.com/uatuko/sentium/pull/91)

### What's Changed
* Handle shutdown signals by @kw510 in https://github.com/uatuko/sentium/pull/74
* Add discussion reference for ReBAC by @Pr301 in https://github.com/uatuko/sentium/pull/76
* Tuples by @uatuko in https://github.com/uatuko/sentium/pull/75
* Fileshare Example: add host:port flag by @td0m in https://github.com/uatuko/sentium/pull/78
* Change Authz service to use tuples instead of records by @uatuko in https://github.com/uatuko/sentium/pull/77
* Bump vite from 5.0.11 to 5.0.12 in /examples/fileshare/app by @dependabot in https://github.com/uatuko/sentium/pull/81
* Bump google.golang.org/protobuf from 1.32.0 to 1.33.0 in /examples/fileshare by @dependabot in https://github.com/uatuko/sentium/pull/80
* Bump golang.org/x/crypto from 0.16.0 to 0.17.0 in /examples/fileshare by @dependabot in https://github.com/uatuko/sentium/pull/79
* Bump vite from 5.0.12 to 5.0.13 in /examples/fileshare/app by @dependabot in https://github.com/uatuko/sentium/pull/83
* Relations gRPC service by @uatuko in https://github.com/uatuko/sentium/pull/82
* List Relations (left) by @uatuko in https://github.com/uatuko/sentium/pull/84
* List relations (right) by @uatuko in https://github.com/uatuko/sentium/pull/86
* Change Resources service to use tuples instead of records by @uatuko in https://github.com/uatuko/sentium/pull/88
* Drop records table by @uatuko in https://github.com/uatuko/sentium/pull/89
* Entities gRPC service by @uatuko in https://github.com/uatuko/sentium/pull/91
* Tidy-up Authz gRPC service by @uatuko in https://github.com/uatuko/sentium/pull/90
* Enable Relations gRPC service by @uatuko in https://github.com/uatuko/sentium/pull/92
* GitHub publish workflow by @uatuko in https://github.com/uatuko/sentium/pull/93
* Command-line options by @uatuko in https://github.com/uatuko/sentium/pull/94
* Publish multi-platform containers by @uatuko in https://github.com/uatuko/sentium/pull/95

### New Contributors
* @Pr301 made their first contribution in https://github.com/uatuko/sentium/pull/76

**Full Changelog**: https://github.com/uatuko/sentium/compare/v0.1.0...v0.2.0


## v0.1.0 - 21st March, 2024

* Initial release.
