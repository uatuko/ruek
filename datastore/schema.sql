create table if not exists collections (
	_id  text    not null,
	_rev integer not null,
	name text    not null,

	constraint "collections.pkey" primary key (_id)
);

create table if not exists identities (
	_id  text    not null,
	_rev integer not null,
	sub  text    not null,

	constraint "identities.pkey" primary key (_id),
	constraint "identities.key-sub" unique (sub)
);

alter table identities add constraint "key-sub" unique (sub);
