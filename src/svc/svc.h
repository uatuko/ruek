#pragma once

#include "authz.h"
#include "principals.h"
#include "relations.h"
#include "resources.h"
#include "wrapper.h"

namespace svc {
using Authz      = Wrapper<authz::Impl>;
using Principals = Wrapper<principals::Impl>;
using Relations  = Wrapper<relations::Impl>;
using Resources  = Wrapper<resources::Impl>;
} // namespace svc
