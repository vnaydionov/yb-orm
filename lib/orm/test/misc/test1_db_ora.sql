-- Oracle SQLPlus script

-- Definition of table test1_db.phonebook
-- drop table phonebook;
create table  phonebook (
  id number not null,
  ts date default sysdate not null,
  name varchar2(100) default null,
  phone varchar2(40) default null,
  primary key (id)
);

-- Dumping data for table test1_db.phonebook
insert into phonebook (id,ts,name,phone) values 
  (1,to_date('2011-03-13 22:37:24','YYYY-MM-DD HH24:MI:SS'),'vasya','123');
insert into phonebook (id,ts,name,phone) values 
  (2,to_date('2011-03-13 22:37:24','YYYY-MM-DD HH24:MI:SS'),'petya','234');
insert into phonebook (id,ts,name,phone) values 
  (3,to_date('2011-03-13 22:37:24','YYYY-MM-DD HH24:MI:SS'),'musya','220');
insert into phonebook (id,ts,name,phone) values 
  (4,to_date('2011-03-13 22:59:31','YYYY-MM-DD HH24:MI:SS'),'zhora','765');
commit;

-- Definition of table test1_db.T_ORM_TEST
-- drop table T_ORM_TEST;
create table  T_ORM_TEST (
  id number not null,
  a varchar2(100) default null,
  b date default sysdate,
  c number default null,
  primary key (id)
);

create sequence S_ORM_TEST_ID;

-- Definition of table test1_db.T_ORM_XML
-- drop table T_ORM_XML;
create table  T_ORM_XML (
  id number not null,
  orm_test_id number not null,
  b number,
  primary key (id)
);

create sequence S_ORM_XML_ID start with 100;

insert into T_ORM_XML (id, orm_test_id, b) values 
  (10, 1, 4);
commit;

