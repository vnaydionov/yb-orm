bin_PROGRAMS = orm_domain_generator
orm_domain_generator_SOURCES = orm_domain_generator.cpp

# set the include path found by configure
INCLUDES= $(all_includes) \
	-I $(top_builddir)/lib

# the library search path.
orm_domain_generator_LDFLAGS = $(all_libraries) \
	$(top_builddir)/lib/util/libybutil.la \
	$(top_builddir)/lib/orm/libyborm.la \
	$(XML_LIBS) \
	-lboost_thread -lboost_date_time \
	-lodbc

