
BUILT_SOURCES=Client.cpp Product.cpp tut1.sql tut4.sql

Client.cpp: $(top_srcdir)/examples/tutorial/tut1.xml
	$(top_builddir)/src/yborm_gen/yborm_gen --domain $^ ./

Product.cpp: $(top_srcdir)/examples/tutorial/tut4.xml
	$(top_builddir)/src/yborm_gen/yborm_gen --domain $^ ./

tut1.sql: $(top_srcdir)/examples/tutorial/tut1.xml
	$(top_builddir)/src/yborm_gen/yborm_gen --ddl $^ ${YBORM_DBTYPE} $@

tut4.sql: $(top_srcdir)/examples/tutorial/tut4.xml
	$(top_builddir)/src/yborm_gen/yborm_gen --ddl $^ ${YBORM_DBTYPE} $@

export YBORM_DBTYPE=@YBORM_DBTYPE@

clean-local:
	-rm -f *.cpp *.h *.sql

