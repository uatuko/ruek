create table if not exists identities (
	_id  text    primary key,
	_rev integer not null,
	sub  text    not null
);

alter table identities add constraint "key-sub" unique (sub);
