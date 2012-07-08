
BUILT_SOURCES=Client.cpp mk_tables.sql

Client.cpp: $(top_srcdir)/examples/ex2_schema.xml
	$(top_builddir)/src/yborm_gen_domain --domain $^ ./

mk_tables.sql: $(top_srcdir)/examples/ex2_schema.xml
	$(top_builddir)/src/yborm_gen_domain --ddl $^ ${YBORM_DBTYPE} $@

export YBORM_DBTYPE=@YBORM_DBTYPE@

clean-local:
	-rm -f *.cpp *.h *.sql

