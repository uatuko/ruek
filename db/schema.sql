create table if not exists principals (
	_rev     integer not null,
	id       text    not null,
	segment  text,
	attrs    jsonb,

	constraint "principals.pkey" primary key (id),
	constraint "principals.check-segment" check (segment <> ''),
	constraint "principals.check-attrs" check (jsonb_typeof(attrs) = 'object')
);

create index "principals.idx-segment" on principals using hash (segment);

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
