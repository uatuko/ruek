#pragma once

#include "basic_error.h"

namespace err {
using DbConnectionUnavailable = basic_error<"sentium:1.0.1.503", "Unavailable">;
using DbTimeout               = basic_error<"sentium:1.0.2.503", "Operation timed out">;
} // namespace err
