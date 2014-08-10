
AM_CXXFLAGS = \
	-I $(top_srcdir)/include/yb \
	-I $(top_srcdir)/include/private \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

lib_LTLIBRARIES = libybutil.la

libybutil_la_SOURCES = \
	data_types.cpp \
	decimal.cpp \
	element_tree.cpp \
	exception.cpp \
	nlogger.cpp \
	string_type.cpp \
	string_utils.cpp \
	thread.cpp \
	utility.cpp \
	value_type.cpp \
	xml_writer.cpp

libybutil_la_LDFLAGS = -version-info 0:0:0

libybutil_la_LIBADD = \
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS)

