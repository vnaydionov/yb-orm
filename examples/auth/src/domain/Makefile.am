
BUILT_SOURCES=LoginSession.cpp auth_schema.sql

LoginSession.cpp: $(top_srcdir)/src/auth_schema.xml
	@YBORM_GEN_DOMAIN@ $^ ./

auth_schema.sql: $(top_srcdir)/src/auth_schema.xml
	@YBORM_GEN_SQL@ ${YBORM_DBTYPE} $^ > $@

export YBORM_DBTYPE=@YBORM_DBTYPE@

clean-local:
	-rm -f *.cpp *.h *.sql

