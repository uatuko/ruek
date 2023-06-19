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
	_id  text    not null,
	_rev integer not null,
	sub  text    not null,

	constraint "identities.pkey" primary key (_id),
	constraint "identities.key-sub" unique (sub),
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

	constraint "roles.pkey" primary key (_id)
);

create table if not exists "collections_access-policies" (
	collection_id text not null,
	policy_id     text not null,

	constraint "collections_access-policies.pkey" primary key (collection_id, policy_id),
	constraint "collections_access-policies.fkey-identity_id" foreign key (collection_id)
		references collections(_id)
		on delete cascade,
	constraint "collections_access-policies.fkey-policy_id" foreign key (policy_id)
		references "access-policies"(_id)
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

create table if not exists "collections_rbac-policies" (
	collection_id text not null,
	policy_id     text not null,

	constraint "collections_rbac-policies.pkey" primary key (collection_id, policy_id),
	constraint "collections_rbac-policies.fkey-identity_id" foreign key (collection_id)
		references collections(_id)
		on delete cascade,
	constraint "collections_rbac-policies.fkey-policy_id" foreign key (policy_id)
		references "rbac-policies"(_id)
		on delete cascade
);

create table if not exists "identities_access-policies" (
	identity_id text not null,
	policy_id   text not null,

	constraint "identities_access-policies.pkey" primary key (identity_id, policy_id),
	constraint "identities_access-policies.fkey-identity_id" foreign key (identity_id)
		references identities(_id)
		on delete cascade,
	constraint "identities_access-policies.fkey-policy_id" foreign key (policy_id)
		references "access-policies"(_id)
		on delete cascade
);

create table if not exists "identities_rbac-policies" (
	identity_id text not null,
	policy_id   text not null,

	constraint "identities_rbac-policies.pkey" primary key (identity_id, policy_id),
	constraint "identities_rbac-policies.fkey-identity_id" foreign key (identity_id)
		references identities(_id)
		on delete cascade,
	constraint "identities_rbac-policies.fkey-policy_id" foreign key (policy_id)
		references "rbac-policies"(_id)
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
