drop table if exists users;
create table users (id integer primary key autoincrement, login text, password text);
insert into users (login,password) values ('ala','ala');
insert into users (login,password) values ('basia','basia');
insert into users (login,password) values ('cyprian','cyprian');

drop table if exists friendship;
create table friendship (id integer primary key autoincrement, user_id integer, friend_id integer);

drop table if exists groups;
create table groups (id integer primary key autoincrement, name text, owner_id integer);

drop table if exists membership;
create table membership (id integer primary key autoincrement, user_id integer, group_id integer);

drop table if exists messages;
create table messages (id integer primary key autoincrement, date integer, sender integer, recipient integer, data text);

drop table if exists confirmations;
create table confirmations (id integer primary key autoincrement, message_id integer, client_uuid text);