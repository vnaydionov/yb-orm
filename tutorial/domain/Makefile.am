
BUILT_SOURCES=Client.cpp tut1.sql

Client.cpp: $(top_srcdir)/tutorial/tut1.xml
	$(top_builddir)/src/yborm_gen_domain --domain $^ ./

tut1.sql: $(top_srcdir)/tutorial/tut1.xml
	$(top_builddir)/src/yborm_gen_domain --ddl $^ ${YBORM_DBTYPE} $@

export YBORM_DBTYPE=@YBORM_DBTYPE@

clean-local:
	-rm -f *.cpp *.h *.sql

