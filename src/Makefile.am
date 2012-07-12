
bin_PROGRAMS = yborm_gen
yborm_gen_SOURCES = yborm_gen.cpp

bin_SCRIPTS = yborm_gen_sql
yborm_datadir = $(prefix)/share/yborm
yborm_data_DATA = yborm_gen_sql.xsl

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

yborm_gen_LDFLAGS = \
	$(top_builddir)/lib/orm/libyborm.la \
	$(top_builddir)/lib/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS) \
	$(DBPOOL3_LDFLAGS) \
	$(DBPOOL3_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS)

