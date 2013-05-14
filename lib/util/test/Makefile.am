
AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(CPPUNIT_CFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

check_PROGRAMS = unit_tests

unit_tests_SOURCES = \
	TestDecimal.cpp \
	TestSingleton.cpp \
	TestResultSet.cpp \
	TestMisc.cpp

unit_tests_LDFLAGS = \
	$(top_builddir)/lib/test/libtestmain.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(CPPUNIT_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS)

TESTS = unit_tests

