create table if not exists "access-policies" (
	_id   text    not null,
	_rev  integer not null,
	rules jsonb,

	constraint "access-policies.pkey" primary key (_id)
);

create table if not exists collections (
	_id  text    not null,
	_rev integer not null,
	name text    not null,

	constraint "collections.pkey" primary key (_id),
	constraint "collections.check-name" check (name <> '')
);

create table if not exists identities (
	_id   text    not null,
	_rev  integer not null,
	sub   text    not null,
	attrs jsonb,

	constraint "identities.pkey" primary key (_id),
	constraint "identities.key-sub" unique (sub),
	constraint "identities.check-attrs" check(jsonb_typeof(attrs) = 'object'),
	constraint "identities.check-sub" check (sub <> '')
);

create table if not exists "rbac-policies" (
	_id   text    not null,
	_rev  integer not null,
	name  text    not null,
	rules jsonb,

	constraint "rbac-policies.pkey" primary key (_id)
);

create table if not exists roles (
	_id         text    not null,
	_rev        integer not null,
	name        text    not null,
	permissions text[],

	constraint "roles.pkey" primary key (_id),
	constraint "roles.check-permissions" check(array_position(permissions, null) = null and array_position(permissions, '') = null)
);


create table if not exists "access-policies_collections" (
	policy_id     text not null,
	collection_id text not null,

	constraint "access-policies_collections.pkey" primary key (policy_id, collection_id),
	constraint "access-policies_collections.fkey-policy_id" foreign key (policy_id)
		references "access-policies"(_id)
		on delete cascade,
	constraint "access-policies_collections.fkey-collection_id" foreign key (collection_id)
		references collections(_id)
		on delete cascade
);

create table if not exists "access-policies_identities" (
	policy_id   text not null,
	identity_id text not null,

	constraint "access-policies_identities.pkey" primary key (policy_id, identity_id),
	constraint "access-policies_identities.fkey-policy_id" foreign key (policy_id)
		references "access-policies"(_id)
		on delete cascade,
	constraint "access-policies_identities.fkey-identity_id" foreign key (identity_id)
		references identities(_id)
		on delete cascade
);

create table if not exists "collections_identities" (
	collection_id text not null,
	identity_id   text not null,

	constraint "collections_identities.pkey" primary key (collection_id, identity_id),
	constraint "collections_identities.fkey-collection_id" foreign key (collection_id)
		references collections(_id)
		on delete cascade,
	constraint "collections_identities.fkey-identity_id" foreign key (identity_id)
		references identities(_id)
		on delete cascade
);

create table if not exists "rbac-policies_collections" (
	policy_id     text not null,
	collection_id text not null,

	constraint "rbac-policies_collections.pkey" primary key (policy_id, collection_id),
	constraint "rbac-policies_collections.fkey-policy_id" foreign key (policy_id)
		references "rbac-policies"(_id)
		on delete cascade,
	constraint "rbac-policies_collections.fkey-collection_id" foreign key (collection_id)
		references collections(_id)
		on delete cascade
);

create table if not exists "rbac-policies_identities" (
	policy_id   text not null,
	identity_id text not null,

	constraint "rbac-policies_identities.pkey" primary key (policy_id, identity_id),
	constraint "rbac-policies_identities.fkey-policy_id" foreign key (policy_id)
		references "rbac-policies"(_id)
		on delete cascade,
	constraint "rbac-policies_identities.fkey-identity_id" foreign key (identity_id)
		references identities(_id)
		on delete cascade
);

create table if not exists "rbac-policies_roles" (
	policy_id text not null,
	role_id   text not null,

	constraint "rbac-policies_roles.pkey" primary key (policy_id, role_id),
	constraint "rbac-policies_roles.fkey-policy_id" foreign key (policy_id)
		references "rbac-policies"(_id)
		on delete cascade,
	constraint "rbac-policies_roles.fkey-role_id" foreign key (role_id)
		references roles(_id)
		on delete cascade
);
