create table if not exists collections (
	_id  text    not null,
	_rev integer not null,
	name text    not null,

	constraint "collections.pkey" primary key (_id)
);

create table if not exists identities (
	_id  text    primary key,
	_rev integer not null,
	sub  text    not null
);

alter table identities add constraint "key-sub" unique (sub);
