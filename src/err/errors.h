#pragma once

#include "basic_error.h"

namespace err {
using DatastoreRevisionMismatch = basic_error<"gk:1.1.1.409", "Revision mismatch">;

using DatastoreDuplicateIdentity = basic_error<"gk:1.2.1.409", "Duplicate identity">;
} // namespace err
