
BUILT_SOURCES=Client.cpp mk_tables.sql

Client.cpp: $(top_srcdir)/examples/DBScheme.xml
	$(top_builddir)/src/orm_domain_generator \
	    $(top_srcdir)/examples/DBScheme.xml ./

mk_tables.sql: $(top_srcdir)/examples/DBScheme.xml
	xsltproc --stringparam dialect ${YBORM_DBTYPE} \
		$(top_srcdir)/src/gen_sql.xsl \
		$(top_srcdir)/examples/DBScheme.xml > $@

export YBORM_DBTYPE=@YBORM_DBTYPE@

clean-local:
	-rm -f *.cpp *.h *.sql

