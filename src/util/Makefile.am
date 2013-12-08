
AM_CXXFLAGS = \
	-I $(top_srcdir)/include/yb \
	-I $(top_srcdir)/include/private \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

lib_LTLIBRARIES = libybutil.la

libybutil_la_SOURCES = \
	string_type.cpp \
	exception.cpp \
	thread.cpp \
	decimal.cpp \
	data_types.cpp \
	string_utils.cpp \
	xml_writer.cpp \
	element_tree.cpp \
	value_type.cpp \
	nlogger.cpp

libybutil_la_LDFLAGS = -version-info 0:0:0

libybutil_la_LIBADD = \
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS)

