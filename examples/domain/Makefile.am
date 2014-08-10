
BUILT_SOURCES=Client.cpp mk_tables.sql

SCHEMA=$(top_srcdir)/examples/ex2_schema.xml

Client.cpp: $(SCHEMA)
	$(top_builddir)/src/yborm_gen/yborm_gen --domain $(SCHEMA) ./

mk_tables.sql: $(SCHEMA)
	$(top_builddir)/src/yborm_gen/yborm_gen --ddl $(SCHEMA) "@YBORM_DBTYPE@" $@

clean-local:
	-rm -f *.cpp *.h *.sql

