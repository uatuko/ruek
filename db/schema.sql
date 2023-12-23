create table if not exists principals (
	_rev      integer not null,
	id        text    not null,
	parent_id text,
	attrs     jsonb,

	constraint "principals.pkey" primary key (id),
	constraint "principals.fkey-parent_id" foreign key (parent_id)
		references principals(id)
		on delete set null,
	constraint "principals.check-attrs" check(jsonb_typeof(attrs) = 'object')
);

create table if not exists records (
	_rev          integer not null,
	principal_id  text    not null,
	resource_type text    not null,
	resource_id   text    not null,
	attrs         jsonb,

	constraint "records.pkey" primary key (principal_id, resource_type, resource_id),
	constraint "records.fkey-principal_id" foreign key (principal_id)
		references principals(id)
		on delete cascade,

	constraint "records.check-resource_type" check (resource_type <> ''),
	constraint "records.check-resource_id" check (resource_id <> ''),
	constraint "records.check-attrs" check (jsonb_typeof(attrs) = 'object')
);
