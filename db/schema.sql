create table if not exists principals (
	_rev      integer not null,
	id        text    not null,
	parent_id text,
	attrs     jsonb,

	constraint "principals.pkey" primary key (id),
	constraint "principals.fkey-parent_id" foreign key (parent_id)
		references principals(id)
		on delete set null,
	constraint "principals.check-attrs" check (jsonb_typeof(attrs) = 'object')
);
