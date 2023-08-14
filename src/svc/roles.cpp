#include "roles.h"

namespace svc {
grpc::ServerUnaryReactor *Roles::AddPermission(
	grpc::CallbackServerContext *context, const gk::v1::RolesAddPermissionRequest *request,
	gk::v1::Role *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto role  = datastore::RetrieveRole(request->role_id());
	auto rules = datastore::ListRbacPolicyRulesByRole(role.id());

	auto permissionId = request->id();
	role.addPermission(permissionId);

	for (const auto &rule : rules) {
		const auto policy = datastore::RetrieveRbacPolicy(rule.policyId);
		for (const auto &iden : policy.identities()) {
			datastore::RbacPolicy::Cache cache({
				.identity   = iden,
				.permission = permissionId,
				.policy     = policy.id(),
				.rule       = rule,
			});
			std::cout << iden << std::endl;

			cache.store();
		}
	}

	map(role, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Roles::Create(
	grpc::CallbackServerContext *context, const gk::v1::RolesCreateRequest *request,
	gk::v1::Role *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto role = map(request);
	role.store();

	for (const auto &perm : request->permission_ids()) {
		role.addPermission(perm);
	}

	map(role, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Roles::Retrieve(
	grpc::CallbackServerContext *context, const gk::v1::RolesRetrieveRequest *request,
	gk::v1::Role *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto role = datastore::RetrieveRole(request->id());
	map(role, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

datastore::Role Roles::map(const gk::v1::RolesCreateRequest *from) {
	datastore::Role role({
		.id   = from->id(),
		.name = from->name(),
	});

	return role;
}

void Roles::map(const datastore::Role &from, gk::v1::Role *to) {
	to->set_id(from.id());
	to->set_name(from.name());

	for (const auto &perm : datastore::RetrievePermissionsByRole(from.id())) {
		auto p = to->add_permissions();
		Permissions::map(perm, p);
	}
}
} // namespace svc
