rem Supported dialects are: MYSQL, INTERBASE, ORACLE, POSTGRES, SQLITE
set DIALECT=MYSQL
set PATH=c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.1\mingw\bin;%PATH%
yborm_gen --ddl test_schema.xml %DIALECT% test_schema.sql
yborm_gen --ddl ex1_schema.xml %DIALECT% ex1_schema.sql
yborm_gen --ddl ex2_schema.xml %DIALECT% ex2_schema.sql
yborm_gen --ddl auth_schema.xml %DIALECT% auth_schema.sql

