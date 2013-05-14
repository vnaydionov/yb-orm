
SUBDIRS=domain

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
	domain/User.cpp \
	domain/LoginSession.cpp \
	auth.cpp \
	tcp_socket.cpp \
	micro_http.cpp \
	App.cpp \
	md5.c

auth_LDFLAGS=\
	$(XML_LIBS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_LIBS_R) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS) \
	$(YBORM_LDFLAGS) \
	$(YBORM_LIBS)

