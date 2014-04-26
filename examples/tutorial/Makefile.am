
SUBDIRS=domain
AUTOMAKE_OPTIONS = subdir-objects

AM_CXXFLAGS = \
	-I $(top_srcdir)/include/yb \
	$(XML_CPPFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(SOCI_CXXFLAGS) \
	$(BOOST_CPPFLAGS)

noinst_PROGRAMS=tut1 tut2 tut3 tut4

tut1_SOURCES=tut1.cpp domain/Client.cpp domain/Order.cpp

tut1_LDFLAGS = \
	$(top_builddir)/src/orm/libyborm.la \
	$(top_builddir)/src/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(EXECINFO_LIBS)

tut2_SOURCES=tut2.cpp domain/Client.cpp domain/Order.cpp

tut2_LDFLAGS = \
	$(top_builddir)/src/orm/libyborm.la \
	$(top_builddir)/src/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(EXECINFO_LIBS)

tut3_SOURCES=tut3.cpp domain/Client.cpp domain/Order.cpp

tut3_LDFLAGS = \
	$(top_builddir)/src/orm/libyborm.la \
	$(top_builddir)/src/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(EXECINFO_LIBS)

tut4_SOURCES=tut4.cpp domain/ProductGroup.cpp domain/Product.cpp

tut4_LDFLAGS = \
	$(top_builddir)/src/orm/libyborm.la \
	$(top_builddir)/src/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(EXECINFO_LIBS)

