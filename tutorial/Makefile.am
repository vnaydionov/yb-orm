
SUBDIRS=domain

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(SOCI_CXXFLAGS) \
	$(BOOST_CPPFLAGS)

noinst_PROGRAMS=tut1 tut2 tut3 tut4

tut1_SOURCES=tut1.cpp domain/Client.cpp domain/Order.cpp

tut1_LDFLAGS = \
	$(top_builddir)/lib/orm/libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(DBPOOL3_LDFLAGS) \
	$(DBPOOL3_LIBS)

tut2_SOURCES=tut2.cpp domain/Client.cpp domain/Order.cpp

tut2_LDFLAGS = \
	$(top_builddir)/lib/orm/libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(DBPOOL3_LDFLAGS) \
	$(DBPOOL3_LIBS)

tut3_SOURCES=tut3.cpp domain/Client.cpp domain/Order.cpp

tut3_LDFLAGS = \
	$(top_builddir)/lib/orm/libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(DBPOOL3_LDFLAGS) \
	$(DBPOOL3_LIBS)

tut4_SOURCES=tut4.cpp domain/ProductGroup.cpp domain/Product.cpp

tut4_LDFLAGS = \
	$(top_builddir)/lib/orm/libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(DBPOOL3_LDFLAGS) \
	$(DBPOOL3_LIBS)

