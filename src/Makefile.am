bin_PROGRAMS = orm_domain_generator
orm_domain_generator_SOURCES = orm_domain_generator.cpp

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(BOOST_CPPFLAGS)

orm_domain_generator_LDFLAGS = \
	$(top_builddir)/lib/orm/libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS)

