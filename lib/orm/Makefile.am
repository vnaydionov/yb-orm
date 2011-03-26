
include $(top_srcdir)/dirs.mk
ybormincludedir=$(ybincludedir)/orm

SUBDIRS = test

AM_CXXFLAGS = -I $(top_srcdir)/lib -I/usr/local/include/dbpool3

lib_LTLIBRARIES = libyborm.la

yborminclude_HEADERS = \
	Filters.h \
	Mapper.h \
	MetaData.h \
	RowData.h \
	Session.h \
	SqlDataSource.h \
	Value.h \
	OdbcSession.h \
	OdbcDriver.h \
	tiodbc.hpp \
	DataObj.h \
	DomainFactory.h \
	DomainFactorySingleton.h \
	MapperSession.h \
	MetaDataSingleton.h \
	XMLMetaDataConfig.h \
	XMLNode.h

	DbpoolSession.h

libyborm_la_SOURCES = \
	$(yborminclude_HEADERS) \
	Filters.cpp \
	Mapper.cpp \
	MetaData.cpp \
	RowData.cpp \
	Session.cpp \
	SqlDataSource.cpp \
	Value.cpp \
	OdbcSession.cpp \
	OdbcDriver.cpp \
	tiodbc.cpp \
	MapperSession.cpp \
	XMLMetaDataConfig.cpp \
	XMLNode.cpp

	DbpoolSession.cpp

libyborm_la_LDFLAGS = -version-info 0:0:0


check_PROGRAMS = unit_tests

unit_tests_LDADD = libyborm.la test/libormtest.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(top_builddir)/lib/test/libtestmain.la -lcppunit \
	$(XML_LIBS) -lboost_thread -lboost_date_time -lodbc

#	-ldl -lmypp3

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

# for DBPool
#export YBORM_DRIVER=mysql
#export YBORM_HOST=localhost
#export YBORM_PORT=3306
#export YBORM_TIMEOUT=10
#export DB_POOL_SIZE=3

TESTS = unit_tests
