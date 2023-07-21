#pragma once

#include "gk/v1/events.grpc.pb.h"

namespace svc {
class Events final : public gk::v1::Events::CallbackService {
public:
	grpc::ServerUnaryReactor *Process(
		grpc::CallbackServerContext *context, const gk::v1::EventsProcessRequest *request,
		gk::v1::EventsProcessResponse *response) override;
};
} // namespace svc
