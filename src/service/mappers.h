#pragma once

#include "datastore/identities.h"
#include "gk/v1/gatekeeper.grpc.pb.h"

namespace service {
datastore::Identity map(const gk::v1::CreateIdentityRequest *from);

void map(const datastore::Identity &from, gk::v1::Identity *to);
} // namespace service
