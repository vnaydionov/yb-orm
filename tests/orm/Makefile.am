
AM_CXXFLAGS = \
	-I $(top_srcdir)/include/yb \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(SOCI_CXXFLAGS) \
	$(CPPUNIT_CFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

check_SCRIPTS = mk_tables.sql

check_PROGRAMS = unit_tests yborm_catch_tests

unit_tests_SOURCES = \
	test_expression.cpp \
	test_schema.cpp \
	test_engine.cpp \
	test_schema_config.cpp \
	test_xmlizer.cpp \
	test_data_object.cpp \
	test_domain_object.cpp

yborm_catch_tests_SOURCES = \
	test_alias.cpp

unit_tests_LDFLAGS = \
	$(top_builddir)/tests/test_main/libtestmain.la \
	$(top_builddir)/src/orm/libyborm.la \
	$(top_builddir)/src/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(CPPUNIT_LIBS) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS) \
	$(EXECINFO_LIBS)

yborm_catch_tests_LDFLAGS = \
	$(top_builddir)/src/orm/libyborm.la \
	$(top_builddir)/src/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(CPPUNIT_LIBS) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS) \
	$(EXECINFO_LIBS)

TESTS = unit_tests_wrapper.sh yborm_catch_tests_wrapper.sh
#TEST_EXTENSIONS = .sh
#SH_LOG_COMPILER = /bin/sh

unit_tests_wrapper.sh: unit_tests.sh
	cp unit_tests.sh $@
	chmod a+x $@

yborm_catch_tests_wrapper.sh: yborm_catch_tests.sh
	cp $(srcdir)/yborm_catch_tests.sh $@
	chmod a+x $@

SCHEMA=$(srcdir)/testdb_schema.xml

mk_tables.sql: $(SCHEMA)
	$(top_builddir)/src/yborm_gen/yborm_gen --ddl $(SCHEMA) "@YBORM_DBTYPE@" $@

clean-local:
	-rm -f mk_tables.sql

