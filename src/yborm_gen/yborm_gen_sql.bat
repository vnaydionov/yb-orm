set BAT_DIR=%~dp0
call %BAT_DIR%..\bin\yborm_env.bat
set BAT_DIR=%~dp0
yborm_gen --ddl %BAT_DIR%testdb_schema.xml %DIALECT% %BAT_DIR%testdb_schema.sql
yborm_gen --ddl %BAT_DIR%ex1_schema.xml %DIALECT% %BAT_DIR%ex1_schema.sql
yborm_gen --ddl %BAT_DIR%ex2_schema.xml %DIALECT% %BAT_DIR%ex2_schema.sql
yborm_gen --ddl %BAT_DIR%auth_schema.xml %DIALECT% %BAT_DIR%auth_schema.sql

