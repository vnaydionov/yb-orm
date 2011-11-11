
include $(top_srcdir)/dirs.mk
ybutilincludedir=$(ybincludedir)/util

if CPPUNIT_PRESENT
SUBDIRS = . test
else
SUBDIRS =
endif

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS)

lib_LTLIBRARIES = libybutil.la

ybutilinclude_HEADERS = \
	str_utils.hpp \
	param.hpp \
	decimal.h \
	xml_writer.h \
	xmlnode.h \
	stacktrace.h \
	Exception.h \
	ResultSet.h \
	UnicodeSupport.h

libybutil_la_SOURCES = \
	$(ybutilinclude_HEADERS) \
	str_utils.cpp \
	decimal.cpp \
	xml_writer.cpp \
	xmlnode.cpp \
	Exception.cpp \
	UnicodeSupport.cpp

libybutil_la_LDFLAGS = -version-info 0:0:0

libybutil_la_LIBADD = \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R)

