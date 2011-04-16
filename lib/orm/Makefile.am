
include $(top_srcdir)/dirs.mk
ybormincludedir=$(ybincludedir)/orm

SUBDIRS = test

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(BOOST_CPPFLAGS)

lib_LTLIBRARIES = libyborm.la

yborminclude_HEADERS = \
	Filters.h \
	Session.h \
	MetaData.h \
	RowData.h \
	EngineBase.h \
	SqlDataSource.h \
	Value.h \
	Engine.h \
	SqlDriver.h \
	tiodbc.hpp \
	DataObj.h \
	DomainFactory.h \
	DomainFactorySingleton.h \
	EngineSession.h \
	MetaDataSingleton.h \
	XMLMetaDataConfig.h \
	XMLNode.h

libyborm_la_SOURCES = \
	$(yborminclude_HEADERS) \
	Filters.cpp \
	Session.cpp \
	MetaData.cpp \
	RowData.cpp \
	EngineBase.cpp \
	SqlDataSource.cpp \
	Value.cpp \
	Engine.cpp \
	SqlDriver.cpp \
	tiodbc.cpp \
	EngineSession.cpp \
	XMLMetaDataConfig.cpp \
	XMLNode.cpp

libyborm_la_LDFLAGS = -version-info 0:0:0


check_PROGRAMS = unit_tests

unit_tests_LDADD = \
	test/libormtest.la \
	libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(top_builddir)/lib/test/libtestmain.la \
	$(CPPUNIT_LIBS) \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS)

unit_tests_SOURCES =

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

TESTS = unit_tests
