create table if not exists principals (
	space_id text    not null,
	id       text    not null,
	segment  text,
	attrs    jsonb,
	_rev     integer not null,

	constraint "principals.pkey" primary key (space_id, id),
	constraint "principals.check-segment" check (segment <> ''),
	constraint "principals.check-attrs" check (jsonb_typeof(attrs) = 'object')
);

create index "principals.idx-segment" on principals using hash (segment);

create table if not exists records (
	space_id      text    not null,
	principal_id  text    not null,
	resource_type text    not null,
	resource_id   text    not null,
	attrs         jsonb,
	_rev          integer not null,

	constraint "records.pkey" primary key (space_id, principal_id, resource_type, resource_id),
	constraint "records.fkey-space_id+principal_id" foreign key (space_id, principal_id)
		references principals(space_id, id)
		on delete cascade,

	constraint "records.check-resource_type" check (resource_type <> ''),
	constraint "records.check-resource_id" check (resource_id <> ''),
	constraint "records.check-attrs" check (jsonb_typeof(attrs) = 'object')
);
