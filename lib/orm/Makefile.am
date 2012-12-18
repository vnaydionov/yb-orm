
include $(top_srcdir)/dirs.mk
ybormincludedir=$(ybincludedir)/orm

if CPPUNIT_PRESENT
SUBDIRS = . test
else
SUBDIRS = 
endif

if QT4_PRESENT
DEFAULT_DRV_H = QtSqlDriver.h
DEFAULT_DRV_CPP = QtSqlDriver.cpp
SQLITE3_DRV_H =
SQLITE3_DRV_CPP =
else
DEFAULT_DRV_H = OdbcDriver.h tiodbc.hpp
DEFAULT_DRV_CPP = OdbcDriver.cpp tiodbc.cpp
if SQLITE3_PRESENT
SQLITE3_DRV_H = SQLiteDriver.h
SQLITE3_DRV_CPP = SQLiteDriver.cpp
endif
endif

if DBPOOL3_PRESENT
DBPOOL_DRV_H = DBPoolDriver.h
DBPOOL_DRV_CPP = DBPoolDriver.cpp
else
DBPOOL_DRV_H =
DBPOOL_DRV_CPP =
endif

yborminclude_HEADERS = \
	$(H_FILES) $(DEFAULT_DRV_H) $(SQLITE3_DRV_H) $(DBPOOL_DRV_H)
libyborm_la_SOURCES = \
	$(CPP_FILES) $(DEFAULT_DRV_CPP) $(SQLITE3_DRV_CPP) $(DBPOOL_DRV_CPP)

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(DBPOOL3_CXXFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

H_FILES = \
	Expression.h \
	MetaData.h \
	Value.h \
	Engine.h \
	SqlDriver.h \
	SqlPool.h \
	DomainObj.h \
	DomainFactory.h \
	DomainFactorySingleton.h \
	MetaDataSingleton.h \
	XMLMetaDataConfig.h \
	XMLizer.h \
	DataObject.h \
	CodeGen.h

CPP_FILES = \
	$(H_FILES) \
	Expression.cpp \
	MetaData.cpp \
	Value.cpp \
	Engine.cpp \
	SqlDriver.cpp \
	SqlPool.cpp \
	XMLMetaDataConfig.cpp \
	XMLizer.cpp \
	DomainFactory.cpp \
	DataObject.cpp \
	DomainObj.cpp \
	CodeGen.cpp

lib_LTLIBRARIES = libyborm.la

libyborm_la_LDFLAGS = -version-info 0:0:0

libyborm_la_LIBADD = \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(DBPOOL3_LDFLAGS) \
	$(DBPOOL3_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS)

