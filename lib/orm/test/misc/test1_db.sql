-- MySQL dump

-- Create schema test1_db
create database if not exists test1_db;
use test1_db;

-- Definition of table test1_db.phonebook
drop table if exists phonebook;
create table  phonebook (
  id int(11) not null auto_increment,
  ts timestamp not null default current_timestamp on update current_timestamp,
  name varchar(100) default null,
  phone varchar(40) default null,
  primary key (id)
) engine=innodb auto_increment=5 default charset=utf8;

-- Dumping data for table test1_db.phonebook
insert into phonebook (id,ts,name,phone) values 
  (1,'2011-03-13 22:37:24','vasya','123');
insert into phonebook (id,ts,name,phone) values 
  (2,'2011-03-13 22:37:24','petya','234');
insert into phonebook (id,ts,name,phone) values 
  (3,'2011-03-13 22:37:24','musya','220');
insert into phonebook (id,ts,name,phone) values 
  (4,'2011-03-13 22:59:31','zhora','765');
commit;

-- Definition of table test1_db.T_ORM_TEST
drop table if exists T_ORM_TEST;
create table  T_ORM_TEST (
  id int(11) not null,
  a varchar(100) default null,
  b timestamp default current_timestamp,
  c decimal(30, 10) default null,
  primary key (id)
) engine=innodb default charset=utf8;

-- Definition of table test1_db.T_ORM_XML
drop table if exists T_ORM_XML;
create table  T_ORM_XML (
  id int(11) not null,
  orm_test_id int(11) not null,
  b decimal(30, 10),
  primary key (id)
) engine=innodb default charset=utf8;

insert into T_ORM_XML (id, orm_test_id, b) values 
  (10, 1, 4);
commit;

-- Create user and grant permissions
-- drop user 'test1'@'localhost';
create user 'test1'@'localhost' identified by 'test1_pwd';
grant all on test1_db.* to 'test1'@'localhost';

