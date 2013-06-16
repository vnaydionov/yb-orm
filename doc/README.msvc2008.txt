
To build YB.ORM from sources you will need:
    - Microsoft Visual Studio C++ 2008 installed
    - CMake 2.8 installed, accessible via %PATH%
    - YB.ORM package for MSVC2008, unpacked in C:\

Issue the following at Visual Studio 2008 command prompt:
    c:
    cd \yborm\build
    build-msvc.bat (or build-msvc-qt.bat)
    cd \yborm\build-auth
    build-msvc.bat (or build-msvc-qt.bat)

To try out the examples you'll need to create a test database.
All examples are set up to use SQLITE dialect by default.
To produce SQL scripts for other databases, edit DIALECT variable 
in examples\yborm_gen_sql.bat script and run the script again.
Use the environment variable YBORM_URL to specify connection to the database.
Look for this variable in corresponding .bat files.
First component of such URL is "dialect+driver://".
Supported access method for SQLITE dialect is through native driver SQLITE,
for other dialects use ODBC driver.
When using QT4-build there are native database drivers available
via QTSQL diver name and QODBC3 driver name is for ODBC.
Then you'll need to apply generated SQL code to your test database and
possibly setup ODBC datasource (DSN) for it via odbcad32.exe.
Below are steps to take to create an empty test database and a test
user account (using SQLite3, MySQL, Postgres or FireBird).


:: SQLite3 ::

Sample URL: sqlite+sqlite://c:/yborm/examples/test1_db
            sqlite+qtsql://c:/yborm/examples/test1_db

This is the default.  The sqlite3 shell is bundled with YB.ORM, see folder "bin".

Apply sql files like this:
>sqlite3 c:\yborm\examples\test1_db < c:\yborm\examples\test_schema.sql
>sqlite3 c:\yborm\examples\test1_db < c:\yborm\examples\ex2_schema.sql


:: MySQL ::

Sample URL: mysql+odbc://test1:test1_pwd@test1_dsn
            mysql+qodbc3://test1:test1_pwd@test1_dsn
            mysql+qtsql://test1:test1_pwd@127.0.0.1:3306/test1_db

You must know the root password for your MySQL database to be able
to manage databases and users.

>mysql -u root -p mysql
Enter password: ********
Welcome to the MySQL monitor.  Commands end with ; or \g.
Your MySQL connection id is 361
Server version: 5.0.51b-community-nt-log MySQL Community Edition (GPL)

Type 'help;' or '\h' for help. Type '\c' to clear the buffer.

mysql> create database test1_db default charset utf8;
Query OK, 1 row affected (0.00 sec)

mysql> create user test1 identified by "test1_pwd";
Query OK, 0 rows affected (0.00 sec)

mysql> grant all on test1_db.* to test1;
Query OK, 0 rows affected (0.00 sec)

mysql> exit
Bye

>mysql -u test1 -ptest1_pwd test1_db < c:\yborm\examples\test_schema.sql
>mysql -u test1 -ptest1_pwd test1_db < c:\yborm\examples\ex2_schema.sql


:: PostgreSQL ::

Sample URL: postgres+odbc://test1:test1_pwd@test1_dsn
            postgres+qodbc3://test1:test1_pwd@test1_dsn
            postgres+qtsql://test1:test1_pwd@127.0.0.1:5432/test1_db

C:\Program Files\PostgreSQL\9.1\bin>createuser -U postgres -h 127.0.0.1 -p 5432 -D -A -P test1
Enter password for new role: test1_pwd
Enter it again: test1_pwd
Shall the new role be allowed to create more new roles? (y/n) n
Password: <your master password>

C:\Program Files\PostgreSQL\9.1\bin>createdb -U postgres -h 127.0.0.1 -p 5432 -O test1 test1_db
Password: <your master password>

C:\Program Files\PostgreSQL\9.1\bin>psql -h 127.0.0.1 -p 5432 -d test1_db -U test1 < \yborm\examples\test_schema.sql
Password for user test1: test1_pwd

NOTICE:  CREATE TABLE / PRIMARY KEY will create implicit index "t_orm_test_pkey" for table "t_orm_test"
CREATE TABLE
NOTICE:  CREATE TABLE / PRIMARY KEY will create implicit index "t_orm_xml_pkey" for table "t_orm_xml"
CREATE TABLE
ALTER TABLE
CREATE SEQUENCE
CREATE SEQUENCE


:: FireBird (InterBase may also apply) ::

Sample URL: interbase+odbc://test1:test1_pwd@test1_dsn

There is default SYSDBA password: masterkey

>gsec -user SYSDBA -password masterkey
GSEC> add test1 -pw test1_pwd
Warning - maximum 8 significant bytes of password used
GSEC> quit

>isql
Use CONNECT or CREATE DATABASE to specify a database
SQL> CREATE DATABASE 'localhost:c:/yborm/examples/test1_db.fdb'
CON> page_size 8192 user 'test1' password 'test1_pwd';
SQL> quit;

>isql -u test1 -p test1_pwd localhost:c:/yborm/examples/test1_db.fdb < c:\yborm\examples\test_schema.sql
>isql -u test1 -p test1_pwd localhost:c:/yborm/examples/test1_db.fdb < c:\yborm\examples\ex2_schema.sql

