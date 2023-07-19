#pragma once

#include "datastore/access-policies.h"
#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/roles.h"
#include "gk/v1/gatekeeper.grpc.pb.h"

namespace service {
datastore::Collection map(const gk::v1::CreateCollectionRequest *from);

void map(const datastore::AccessPolicy &from, gk::v1::Policy *to);
void map(const datastore::Collection &from, gk::v1::Collection *to);
void map(const datastore::Identity &from, gk::v1::Identity *to);
} // namespace service
