drop table if exists users;
create table users (id integer primary key autoincrement, login text, password text);
insert into users (login,password) values ('earth','earth');
insert into users (login,password) values ('mars','mars');
insert into users (login,password) values ('jupyter','jupyter');

drop table if exists messages;
create table messages (id integer primary key autoincrement, date integer, sender integer, recipient integer, data text, status integer);
