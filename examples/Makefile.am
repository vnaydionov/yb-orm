
SUBDIRS=domain

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(BOOST_CPPFLAGS)

noinst_PROGRAMS=example1
example1_SOURCES=example1.cpp domain/Client.cpp domain/Order.cpp
example1_LDFLAGS = \
	$(top_builddir)/lib/orm/libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS)

