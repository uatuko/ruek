#pragma once

#include "basic_error.h"

namespace err {
using DatastoreRevisionMismatch = basic_error<"gk:1.1.1.409", "Revision mismatch">;

using DatastoreDuplicateIdentity = basic_error<"gk:1.2.1.409", "Duplicate identity">;

using DatastoreCollectionNotFound = basic_error<"gk:1.3.3.404", "Collection not found">;
using DatastoreDuplicateCollectionMember =
	basic_error<"gk:1.3.2.409", "Duplicate collection member">;
using DatastoreInvalidCollectionOrMember =
	basic_error<"gk:1.3.1.400", "Invalid collection or member">;
} // namespace err
