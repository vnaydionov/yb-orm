
bin_PROGRAMS = yborm_gen
yborm_gen_SOURCES = yborm_gen.cpp

AM_CXXFLAGS = \
	-I $(top_srcdir)/include/yb \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(SOCI_CXXFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

yborm_gen_LDFLAGS = \
	$(top_builddir)/src/orm/libyborm.la \
	$(top_builddir)/src/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS) \
	$(EXECINFO_LIBS)

