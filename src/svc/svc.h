#pragma once

#include "authz.h"
#include "entities.h"
#include "principals.h"
#include "relations.h"
#include "wrapper.h"

namespace svc {
using Authz      = Wrapper<authz::Impl>;
using Entities   = Wrapper<entities::Impl>;
using Principals = Wrapper<principals::Impl>;
using Relations  = Wrapper<relations::Impl>;
} // namespace svc
