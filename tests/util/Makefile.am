
AM_CXXFLAGS = \
	-I $(top_srcdir)/include/yb \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(CPPUNIT_CFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

check_PROGRAMS = unit_tests

unit_tests_SOURCES = \
	test_decimal.cpp \
	test_singleton.cpp \
	test_result_set.cpp \
	test_misc.cpp \
	test_value_type.cpp

unit_tests_LDFLAGS = \
	$(top_builddir)/tests/test_main/libtestmain.la \
	$(top_builddir)/src/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(CPPUNIT_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS)

TESTS = unit_tests

