#pragma once

#include "basic_error.h"

namespace err {
using DatastoreRedisCommandError          = basic_error<"gk:1.0.4.503", "Unavailable">;
using DatastoreRedisConnectionFailure     = basic_error<"gk:1.0.1.503", "Unavailable">;
using DatastoreRedisConnectionUnavailable = basic_error<"gk:1.0.3.503", "Unavailable">;
using DatastoreRedisTimeout               = basic_error<"gk:1.0.2.503", "Operation timed out">;

using DatastoreRevisionMismatch = basic_error<"gk:1.1.1.409", "Revision mismatch">;

using DatastoreDuplicateIdentity   = basic_error<"gk:1.2.1.409", "Duplicate identity">;
using DatastoreIdentityNotFound    = basic_error<"gk:1.2.2.404", "Identity not found">;
using DatastoreInvalidIdentityData = basic_error<"gk:1.2.3.400", "Invalid identity data">;

using DatastoreAccessPolicyNotFound =
	basic_error<"gk:1.3.4.404", "Access policy not found">; // TODO: is this number significant?

using DatastoreCollectionNotFound = basic_error<"gk:1.3.3.404", "Collection not found">;
using DatastoreDuplicateCollectionMember =
	basic_error<"gk:1.3.2.409", "Duplicate collection member">;
using DatastoreInvalidCollectionOrMember =
	basic_error<"gk:1.3.1.400", "Invalid collection or member">;

using DatastoreRoleNotFound = basic_error<"gk:1.3.1.404", "Role not found">;

using EventsPubsubTransportFailure = basic_error<"gk:2.1.1.503", "Unavailable">;
} // namespace err
