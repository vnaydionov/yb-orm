
SUBDIRS=domain
AUTOMAKE_OPTIONS = subdir-objects

AM_CXXFLAGS=\
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(SOCI_CXXFLAGS) \
	$(YBORM_CXXFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

bin_PROGRAMS=auth
bin_SCRIPTS=auth.sh

auth_SOURCES=\
	auth.cpp \
	tcp_socket.cpp \
	micro_http.cpp \
	app_class.cpp \
	md5.c

#	domain/User.cpp \
#	domain/LoginSession.cpp \
#

auth_LDFLAGS=\
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS) \
	$(EXECINFO_LIBS) \
	$(YBORM_LDFLAGS) \
	$(YBORM_LIBS)

