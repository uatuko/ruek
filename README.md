# 🔐 Gatekeeper

[![license](https://img.shields.io/badge/license-MIT-green)](https://raw.githubusercontent.com/uatuko/gatekeeper/main/LICENSE)

> ⚠️ This branch is deprecated.

## Why is this branch deprecated?

Simple answer is, it became too complicated and confusing.

### Problem 1: Unnecessary overhead of having to create policies

It's required to create a policy, either an Access Policy or an RBAC Policy, to grant any form of "permission".
This meant even for simple access entires (e.g. user -> [can access] -> document) a policy is required.

While this isn't necessarily a bad thing (and policies could give better visibility of "intentions"), at the data
level it made it unnecessarily complicated. Specially for simple entries, having a policy in Postgres and cache in
Redis seems unnecessary.

### Problem 2: Concerns over cache re-builds

The data is split between Postgres (for persistent storage) and Redis (for cache).

The reasoning for splitting data between Postgres and Redis was initially to optimise for speed. However, this came
with the overhead of having to maintain and more importantly _sync_ two datastores.

In a happy scenario, everything should work and Redis can be eventualy consistent with Postgres. But in a disaster
recovery scenario this adds more complexity and without a quick way to identify which records are out of sync the
cache re-build times increase with the amout of data.

### Problem 3: Conflicting Access and RBAC policy features

The split for Access and RBAC policies was intended to give applications the choice. However, this didn't have the
desired outcome and made it complicated.

Take this scenario for example - an application has different features that require RBAC permissions to grant users access.
(e.g. "User Admin" role to grant users access to the user management feature). This can be achieved by RBAC policies.

Access policies can then work along side the RBAC policies to grant users more granular permissions (e.g. Users with "User Admin"
role can only manage users with an explicit grant from Access policies).

Now let's expand this to consider multi-tenancy (or even simply multi-project). A user is granted access to two projects,
Project A and Project B. In Project A, the user needs to be given the "User Admin" role but not on Project B. Simply creating an
RBAC policy won't be enough since it's required to restrict access at a project leve. This is where Access and RBAC
policy features conflict - an Access Policy could be used with attributes to indicate which permissions a user is
granted or an RBAC Policy with attributes could be used to indicate the resource restrictions (essentially making it ABAC).

### Problem 4: Too many similar concepts creating unnecessary complexities

Apart from Access Polices an RBAC Policies being very similar (and providing conflicting features), Collections and Identities
are both very similar concepts which adds unnecessary complexity to data structures and code.

### Problem 5: Handicapped gRPC C++ implementations

It's been difficult and frustrating to work with gRPC C++ implementations (more details in [this article](https://medium.com/@u-a/frustrations-of-creating-a-grpc-server-in-c-5c57cbd65c53)).

In hindsight, it's the wrong choice.


## What's next?

A fresh implementation, which should be much simpler and avoid/solve the problems mentioned above, is in the works.

It'll also have a new name 😉.
