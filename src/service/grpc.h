#pragma once

#include "protos/gk/v1/gatekeeper.grpc.pb.h"

namespace service {
class Grpc final : public gk::v1::Gatekeeper::CallbackService {};
} // namespace service
