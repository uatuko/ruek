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

create table if not exists tuples (
	space_id  text not null,  -- used for creating data silos to support multi-tenancy
	strand    text not null,  -- a relation that connects two tuples together

	-- Left entity
	--
	l_entity_type  text not null,
	l_entity_id    text not null,

	relation  text not null,

	-- Right entity
	--
	r_entity_type  text not null,
	r_entity_id    text not null,

	attrs  jsonb,

	-- Duplicate columns for foreign key constraints if either left/right entity refer to a principal
	--
	l_principal_id  text,
	r_principal_id  text,

	_id   text    not null,
	_rid  text,  -- self reference for computed tuples
	_rev  integer not null,

	constraint "tuples.pkey" primary key (_id),
	constraint "tuples.unique" unique (
		space_id,
		l_entity_type, l_entity_id,
		relation,
		r_entity_type, r_entity_id,
		strand),

	constraint "tuples.fkey-_rid" foreign key (_rid)
		references tuples(_id)
		on delete cascade,

	constraint "tuples.fkey-space_id+l_principal_id" foreign key (space_id, l_principal_id)
		references principals(space_id, id)
		on delete cascade,

	constraint "tuples.fkey-space_id+r_principal_id" foreign key (space_id, r_principal_id)
		references principals(space_id, id)
		on delete cascade,

	constraint "tuples.check-l_entity_id" check (l_entity_id <> ''),
	constraint "tuples.check-r_entity_id" check (r_entity_id <> ''),
	constraint "tuples.check-attrs" check (jsonb_typeof(attrs) = 'object')
);
