
AM_CXXFLAGS = \
	-I $(top_srcdir)/include/yb \
	-I $(top_srcdir)/include/private \
	$(XML_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(ODBC_CFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(SOCI_CXXFLAGS) \
	$(WX_CFLAGS) \
	$(QT_CFLAGS)

if QT_PRESENT
QTSQL_DRV_CPP = driver_qtsql.cpp
else
QTSQL_DRV_CPP =
endif

if ODBC_PRESENT
ODBC_DRV_CPP = driver_odbc.cpp tiodbc.cpp
else
ODBC_DRV_CPP =
endif

if SQLITE3_PRESENT
SQLITE3_DRV_CPP = driver_sqlite.cpp
else
SQLITE3_DRV_CPP =
endif

if SOCI_PRESENT
SOCI_DRV_CPP = driver_soci.cpp
else
SOCI_DRV_CPP =
endif

CPP_FILES = \
	alias.cpp \
	code_gen.cpp \
	data_object.cpp \
	domain_factory.cpp \
	domain_object.cpp \
	engine.cpp \
	expression.cpp \
	schema_config.cpp \
	schema.cpp \
	schema_reader.cpp \
	sql_driver.cpp \
	sql_pool.cpp \
	xmlizer.cpp \
	dialect_sqlite.cpp \
	dialect_oracle.cpp \
	dialect_postgres.cpp \
	dialect_mysql.cpp \
	dialect_mssql.cpp \
	dialect_interbase.cpp

libyborm_la_SOURCES = \
	$(CPP_FILES) \
	$(QTSQL_DRV_CPP) $(ODBC_DRV_CPP) $(SQLITE3_DRV_CPP) $(SOCI_DRV_CPP)

lib_LTLIBRARIES = libyborm.la

libyborm_la_LDFLAGS = -version-info 0:0:0

libyborm_la_LIBADD = \
	$(top_builddir)/src/util/libybutil.la \
	$(XML_LIBS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(BOOST_THREAD_LIBS) $(BOOST_DATE_TIME_LIBS) \
	$(ODBC_LIBS) \
	$(SQLITE3_LIBS) \
	$(SOCI_LIBS) \
	$(WX_LIBS) \
	$(QT_LDFLAGS) \
	$(QT_LIBS)

