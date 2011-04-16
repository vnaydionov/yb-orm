
include $(top_srcdir)/dirs.mk
ybutilincludedir=$(ybincludedir)/util

SUBDIRS = test

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(BOOST_CPPFLAGS)

lib_LTLIBRARIES = libybutil.la

ybutilinclude_HEADERS = \
	str_utils.hpp param.hpp decimal.h xml_writer.h xmlnode.h

libybutil_la_SOURCES = \
	$(ybutilinclude_HEADERS) \
	str_utils.cpp decimal.cpp xml_writer.cpp xmlnode.cpp

libybutil_la_LDFLAGS = -version-info 0:0:0


check_PROGRAMS = unit_tests

unit_tests_LDADD = \
	test/libutiltest.la \
	libybutil.la \
	$(top_builddir)/lib/test/libtestmain.la \
	$(CPPUNIT_LIBS) \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R)

unit_tests_SOURCES =

TESTS = unit_tests
