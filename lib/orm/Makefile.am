
include $(top_srcdir)/dirs.mk
ybormincludedir=$(ybincludedir)/orm

if CPPUNIT_PRESENT
SUBDIRS = . test
else
SUBDIRS = 
endif

if DBPOOL3_PRESENT
yborminclude_HEADERS = $(H_FILES) DBPoolDriver.h
libyborm_la_SOURCES = $(CPP_FILES) DBPoolDriver.cpp
else
yborminclude_HEADERS = $(H_FILES)
libyborm_la_SOURCES = $(CPP_FILES)
endif

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(DBPOOL3_CXXFLAGS)

H_FILES = \
	Filters.h \
	MetaData.h \
	Value.h \
	Engine.h \
	SqlDriver.h \
	tiodbc.hpp \
	DomainObj.h \
	DomainFactory.h \
	DomainFactorySingleton.h \
	MetaDataSingleton.h \
	XMLMetaDataConfig.h \
	XMLNode.h \
	DataObject.h

CPP_FILES = \
	$(H_FILES) \
	Filters.cpp \
	MetaData.cpp \
	Value.cpp \
	Engine.cpp \
	SqlDriver.cpp \
	tiodbc.cpp \
	XMLMetaDataConfig.cpp \
	XMLNode.cpp \
	DomainFactory.cpp \
	DataObject.cpp \
	DomainObj.cpp

lib_LTLIBRARIES = libyborm.la

libyborm_la_LDFLAGS = -version-info 0:0:0

libyborm_la_LIBADD = \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS) \
	$(DBPOOL3_LDFLAGS) \
	$(DBPOOL3_LIBS)

