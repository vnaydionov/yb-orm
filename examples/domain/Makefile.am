
BUILT_SOURCES=Client.cpp mk_tables.sql

Client.cpp: $(top_srcdir)/examples/DBScheme.xml
	$(top_builddir)/src/orm_domain_generator \
	    $(top_srcdir)/examples/DBScheme.xml ./

mk_tables.sql: $(top_srcdir)/examples/DBScheme.xml
	xsltproc $(top_srcdir)/src/gen_sql.xsl \
	    $(top_srcdir)/examples/DBScheme.xml > $@

