
SUBDIRS=domain

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

noinst_PROGRAMS=example1 example2

example1_SOURCES=example1.cpp domain/Client.cpp domain/Order.cpp \
	domain/Product.cpp domain/ProductGroup.cpp domain/Contract.cpp \
	domain/OrderItem.cpp

example1_LDFLAGS = \
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

example2_SOURCES=example2.cpp domain/Client.cpp domain/Order.cpp \
	domain/Product.cpp domain/ProductGroup.cpp domain/Contract.cpp \
	domain/OrderItem.cpp

example2_LDFLAGS = \
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

