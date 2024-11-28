#pragma once

#include "principals.h"
#include "relations.h"
#include "wrapper.h"

namespace svc {
using Principals = Wrapper<principals::Impl>;
using Relations  = Wrapper<relations::Impl>;
} // namespace svc
