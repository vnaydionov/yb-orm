
BUILT_SOURCES=Client.cpp tut1.sql

Client.cpp: $(top_srcdir)/tutorial/tut1.xml
	$(top_builddir)/src/yborm_gen_domain $^ ./

tut1.sql: $(top_srcdir)/tutorial/tut1.xml
	xsltproc --stringparam dialect ${YBORM_DBTYPE} \
		$(top_srcdir)/src/yborm_gen_sql.xsl $^ > $@

export YBORM_DBTYPE=@YBORM_DBTYPE@

clean-local:
	-rm -f *.cpp *.h *.sql

