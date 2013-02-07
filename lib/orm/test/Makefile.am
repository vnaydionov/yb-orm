
include $(top_srcdir)/dirs.mk

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(SOCI_CXXFLAGS) \
	$(CPPUNIT_CFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

check_SCRIPTS = mk_tables.sql

check_PROGRAMS = unit_tests

unit_tests_SOURCES = \
	TestExpression.cpp \
	TestMetaData.cpp \
	TestValue.cpp \
	TestEngine.cpp \
	TestXMLConfig.cpp \
	TestXMLNode.cpp \
	TestDataObject.cpp

unit_tests_LDFLAGS = \
	$(top_builddir)/lib/test/libtestmain.la \
	$(top_builddir)/lib/orm/libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(CPPUNIT_LIBS) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(DBPOOL3_LDFLAGS) \
	$(DBPOOL3_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS)

TESTS = unit_tests

export YBORM_DBTYPE=@YBORM_DBTYPE@
export YBORM_DB=@YBORM_DB@
export YBORM_USER=@YBORM_USER@
export YBORM_PASSWD=@YBORM_PASSWD@
export YBORM_URL=@YBORM_URL@

# Oracle specific
export TWO_TASK=balance-dev.yandex.ru
export ORACLE_HOME=/opt/oracle/instantclient_10_2
export LD_LIBRARY_PATH=/opt/oracle/instantclient_10_2
export NLS_NUMERIC_CHARACTERS=. 
export NLS_SORT=RUSSIAN
export NLS_LANG=AMERICAN_CIS.UTF8

mk_tables.sql: $(srcdir)/test_schema.xml
	$(top_builddir)/src/yborm_gen --ddl $^ ${YBORM_DBTYPE} $@

clean-local:
	-rm -f mk_tables.sql

