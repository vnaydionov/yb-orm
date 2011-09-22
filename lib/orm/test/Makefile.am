
include $(top_srcdir)/dirs.mk

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(CPPUNIT_CFLAGS)

BUILT_SOURCES=mk_tables.sql

check_PROGRAMS = unit_tests

unit_tests_SOURCES = \
	TestFilters.cpp \
	TestSession.cpp \
	TestMetaData.cpp \
	TestRowData.cpp \
	TestValue.cpp \
	TestEngine.cpp \
	TestSqlDataSource.cpp \
	TestXMLConfig.cpp \
	TestIntegrMapper.cpp \
	TestXMLNode.cpp \
	TestDataObject.cpp

unit_tests_LDADD = \
	$(top_builddir)/lib/test/libtestmain.la \
	$(top_builddir)/lib/orm/libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(CPPUNIT_LIBS) \
	$(ODBC_LIBS) \
	$(DBPOOL3_LDFLAGS) \
	$(DBPOOL3_LIBS)

TESTS = unit_tests

export YBORM_DBTYPE=@YBORM_DBTYPE@
export YBORM_DB=@YBORM_DB@
export YBORM_USER=@YBORM_USER@
export YBORM_PASSWD=@YBORM_PASSWD@

# Oracle specific
export TWO_TASK=balance-dev.yandex.ru
export ORACLE_HOME=/opt/oracle/instantclient_10_2
export LD_LIBRARY_PATH=/opt/oracle/instantclient_10_2
export NLS_NUMERIC_CHARACTERS=. 
export NLS_SORT=RUSSIAN
export NLS_LANG=AMERICAN_CIS.UTF8

mk_tables.sql: $(srcdir)/test_schema.xml
	xsltproc --stringparam dialect ${YBORM_DBTYPE} \
		$(top_srcdir)/src/gen_sql.xsl $^ > $@

clean-local:
	-rm -f mk_tables.sql

