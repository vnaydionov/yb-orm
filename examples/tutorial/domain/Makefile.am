
BUILT_SOURCES=Client.cpp Product.cpp tut1.sql tut4.sql

SCHEMA1=$(top_srcdir)/examples/tutorial/tut1.xml
SCHEMA4=$(top_srcdir)/examples/tutorial/tut4.xml

Client.cpp: $(SCHEMA1)
	$(top_builddir)/src/yborm_gen/yborm_gen --domain $(SCHEMA1) ./

Product.cpp: $(SCHEMA4)
	$(top_builddir)/src/yborm_gen/yborm_gen --domain $(SCHEMA4) ./

tut1.sql: $(SCHEMA1)
	$(top_builddir)/src/yborm_gen/yborm_gen --ddl $(SCHEMA1) "@YBORM_DBTYPE@" $@

tut4.sql: $(SCHEMA4)
	$(top_builddir)/src/yborm_gen/yborm_gen --ddl $(SCHEMA4) "@YBORM_DBTYPE@" $@

clean-local:
	-rm -f *.cpp *.h *.sql

