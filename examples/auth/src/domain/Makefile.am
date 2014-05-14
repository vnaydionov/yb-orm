
BUILT_SOURCES=LoginSession.cpp auth_schema.sql

SCHEMA=$(top_srcdir)/src/auth_schema.xml

LoginSession.cpp: $(SCHEMA)
	@YBORM_GEN@ --domain $(SCHEMA) ./

auth_schema.sql: $(SCHEMA)
	@YBORM_GEN@ --ddl $(SCHEMA) "@YBORM_DBTYPE@" $@

clean-local:
	-rm -f *.cpp *.h *.sql

