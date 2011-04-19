
include $(top_srcdir)/dirs.mk

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(CPPUNIT_CFLAGS)

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
	TestXMLNode.cpp

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

export YBORM_DBTYPE=MYSQL
export YBORM_DB=test1_db
export YBORM_USER=test1
export YBORM_PASSWD=test1_pwd

#export YBORM_DBTYPE=ORACLE
#export YBORM_DB=test1_db_ora
#export YBORM_USER=bo
#export YBORM_PASSWD=balalancing

# Oracle specific
export TWO_TASK=balance-dev.yandex.ru
export ORACLE_HOME=/opt/oracle/instantclient_10_2
export LD_LIBRARY_PATH=/opt/oracle/instantclient_10_2
export NLS_NUMERIC_CHARACTERS=. 
export NLS_SORT=RUSSIAN
export NLS_LANG=AMERICAN_CIS.UTF8

