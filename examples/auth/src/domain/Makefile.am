
BUILT_SOURCES=LoginSession.cpp auth_schema.sql

LoginSession.cpp: $(top_srcdir)/src/auth_schema.xml
	@YBORM_GEN_DOMAIN@ --domain $^ ./

auth_schema.sql: $(top_srcdir)/src/auth_schema.xml
	@YBORM_GEN_DOMAIN@ --ddl $^ ${YBORM_DBTYPE} $@

export YBORM_DBTYPE=@YBORM_DBTYPE@

clean-local:
	-rm -f *.cpp *.h *.sql

