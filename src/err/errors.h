#pragma once

#include "basic_error.h"

namespace err {
using DbConnectionUnavailable = basic_error<"ruek:1.0.1.503", "Unavailable">;
using DbTimeout               = basic_error<"ruek:1.0.2.503", "Operation timed out">;

using DbRevisionMismatch = basic_error<"ruek:1.1.1.409", "Revision mismatch">;

using DbPrincipalInvalidData = basic_error<"ruek:1.2.1.400", "Invalid principal data">;
using DbPrincipalNotFound    = basic_error<"ruek:1.2.2.404", "Principal not found">;

using DbRecordInvalidData        = basic_error<"ruek:1.3.1.400", "Invalid principal data">;
using DbRecordInvalidPrincipalId = basic_error<"ruek:1.3.2.400", "Invalid principal for record">;

using DbTupleAlreadyExists = basic_error<"ruek:1.4.4.409", "Tuple already exists">;
using DbTupleInvalidData   = basic_error<"ruek:1.4.1.400", "Invalid tuple data">;
using DbTupleNotFound      = basic_error<"ruek:1.4.3.404", "Tuple not found">;

using DbTuplesInvalidListArgs =
	basic_error<"ruek:1.4.4.400", "Invalid arguments for listing tuples">;

using DbTupletsInvalidListArgs =
	basic_error<"ruek:1.4.5.400", "Invalid arguments for listing tuplets">;

using RpcPrincipalsAlreadyExists = basic_error<"ruek:2.1.1.409", "Principal already exists">;
using RpcPrincipalsNotFound      = basic_error<"ruek:2.1.2.404", "Principal not found">;

using RpcRelationsInvalidStrategy = basic_error<"ruek:2.2.1.400", "Invalid relations strategy">;
} // namespace err
