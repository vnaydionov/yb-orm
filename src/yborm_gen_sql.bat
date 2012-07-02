rem Supported dialects are: MYSQL, INTERBASE, ORACLE
set DIALECT=MYSQL
set PATH=c:\yborm\bin;%PATH%
xsltproc --stringparam dialect %DIALECT% yborm_gen_sql.xsl test_schema.xml > test_schema.sql
xsltproc --stringparam dialect %DIALECT% yborm_gen_sql.xsl ex1_schema.xml > ex1_schema.sql
xsltproc --stringparam dialect %DIALECT% yborm_gen_sql.xsl ex2_schema.xml > ex2_schema.sql
xsltproc --stringparam dialect %DIALECT% yborm_gen_sql.xsl auth_schema.xml > auth_schema.sql
