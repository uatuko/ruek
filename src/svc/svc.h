#pragma once

#include "authz.h"
#include "principals.h"
#include "wrapper.h"

namespace svc {
using Authz      = Wrapper<authz::Impl>;
using Principals = Wrapper<principals::Impl>;
} // namespace svc
