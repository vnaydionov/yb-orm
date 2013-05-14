
ybormincludedir=$(includedir)/yb/orm

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
SOCI_DRV_H =
SOCI_DRV_CPP =
else
DEFAULT_DRV_H = OdbcDriver.h tiodbc.hpp
DEFAULT_DRV_CPP = OdbcDriver.cpp tiodbc.cpp
if SQLITE3_PRESENT
SQLITE3_DRV_H = SQLiteDriver.h
SQLITE3_DRV_CPP = SQLiteDriver.cpp
endif
if SOCI_PRESENT
SOCI_DRV_H = SOCIDriver.h
SOCI_DRV_CPP = SOCIDriver.cpp
endif
endif

yborminclude_HEADERS = \
	$(H_FILES) $(DEFAULT_DRV_H) $(SQLITE3_DRV_H) $(SOCI_DRV_H)

libyborm_la_SOURCES = \
	$(CPP_FILES) $(DEFAULT_DRV_CPP) $(SQLITE3_DRV_CPP) $(SOCI_DRV_CPP)

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(SOCI_CXXFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

H_FILES = \
	Expression.h \
	MetaData.h \
	Engine.h \
	SqlDriver.h \
	SqlPool.h \
	DomainObject.h \
	DomainFactory.h \
	XMLMetaDataConfig.h \
	XMLizer.h \
	DataObject.h \
	CodeGen.h

CPP_FILES = \
	$(H_FILES) \
	Expression.cpp \
	MetaData.cpp \
	Engine.cpp \
	SqlDriver.cpp \
	SqlPool.cpp \
	XMLMetaDataConfig.cpp \
	XMLizer.cpp \
	DomainFactory.cpp \
	DataObject.cpp \
	DomainObject.cpp \
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
	$(SOCI_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS)

