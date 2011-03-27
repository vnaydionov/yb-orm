
SUBDIRS=domain

AM_CXXFLAGS = -I $(top_srcdir)/lib

noinst_PROGRAMS=example1
example1_SOURCES=example1.cpp domain/Client.cpp domain/Order.cpp
example1_LDFLAGS = $(all_libraries) \
	$(top_builddir)/lib/util/libybutil.la \
	$(top_builddir)/lib/orm/libyborm.la \
	$(XML_LIBS) \
	-lboost_thread -lboost_date_time \
	-lodbc

