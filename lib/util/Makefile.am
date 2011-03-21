
include $(top_srcdir)/dirs.mk
ybutilincludedir=$(ybincludedir)/util

SUBDIRS = test

AM_CXXFLAGS = -I $(top_srcdir)/lib

lib_LTLIBRARIES = libybutil.la

ybutilinclude_HEADERS = \
	str_utils.hpp param.hpp decimal.h xml_writer.h xmlnode.h

libybutil_la_SOURCES = \
	$(ybutilinclude_HEADERS) \
	str_utils.cpp decimal.cpp xml_writer.cpp xmlnode.cpp

libybutil_la_LDFLAGS = -version-info 0:0:0


check_PROGRAMS = unit_tests

unit_tests_LDADD = libybutil.la test/libutiltest.la \
	$(top_builddir)/lib/test/libtestmain.la -lcppunit \
	$(XML_LIBS) -lboost_thread-mt

unit_tests_SOURCES =

TESTS = unit_tests
