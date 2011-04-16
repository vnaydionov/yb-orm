
include $(top_srcdir)/dirs.mk

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(BOOST_CPPFLAGS) \
	$(CPPUNIT_CFLAGS)

ybtestlib_LTLIBRARIES = libutiltest.la

libutiltest_la_SOURCES = TestDecimal.cpp TestSingleton.cpp
