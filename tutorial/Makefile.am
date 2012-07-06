
SUBDIRS=domain

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS)

noinst_PROGRAMS=tut1 tut2

tut1_SOURCES=tut1.cpp domain/Client.cpp domain/Order.cpp

tut1_LDFLAGS = \
	$(top_builddir)/lib/orm/libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS) \
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
	$(DBPOOL3_LDFLAGS) \
	$(DBPOOL3_LIBS)

