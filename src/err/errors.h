#pragma once

#include "basic_error.h"

namespace err {
using DbConnectionUnavailable = basic_error<"sentium:1.0.1.503", "Unavailable">;
using DbTimeout               = basic_error<"sentium:1.0.2.503", "Operation timed out">;

using DbRevisionMismatch = basic_error<"sentium:1.1.1.409", "Revision mismatch">;

using DbPrincipalInvalidData     = basic_error<"sentium:1.2.1.400", "Invalid principal data">;
using DbPrincipalInvalidParentId = basic_error<"sentium:1.2.2.400", "Invalid parent for principal">;
using DbPrincipalNotFound        = basic_error<"sentium:1.2.3.404", "Principal not found">;

using DbRecordInvalidData        = basic_error<"sentium:1.3.1.400", "Invalid principal data">;
using DbRecordInvalidPrincipalId = basic_error<"sentium:1.3.2.400", "Invalid principal for record">;

using RpcPrincipalsAlreadyExists = basic_error<"sentium:2.1.1.409", "Principal already exists">;
using RpcPrincipalsNotFound      = basic_error<"sentium:2.1.2.404", "Principal not found">;
} // namespace err
