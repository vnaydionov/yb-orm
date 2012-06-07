
BUILT_SOURCES=Client.cpp mk_tables.sql

Client.cpp: $(top_srcdir)/examples/ex2_schema.xml
	$(top_builddir)/src/yborm_gen_domain $^ ./

mk_tables.sql: $(top_srcdir)/examples/ex2_schema.xml
	xsltproc --stringparam dialect ${YBORM_DBTYPE} \
		$(top_srcdir)/src/yborm_gen_sql.xsl $^ > $@

export YBORM_DBTYPE=@YBORM_DBTYPE@

clean-local:
	-rm -f *.cpp *.h *.sql

