#pragma once

#include "principals.h"
#include "wrapper.h"

namespace svc {
using Principals = Wrapper<principals::Impl>;
} // namespace svc
