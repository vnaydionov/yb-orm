
include $(top_srcdir)/dirs.mk

AM_CXXFLAGS = \
	-I $(top_srcdir)/lib \
	$(BOOST_CPPFLAGS)

ybtestlib_LTLIBRARIES = libormtest.la

libormtest_la_SOURCES = \
	TestFilters.cpp \
	TestMapper.cpp \
	TestMetaData.cpp \
	TestRowData.cpp \
	TestValue.cpp \
	TestEngine.cpp \
	TestSqlDataSource.cpp \
	TestXMLConfig.cpp \
	TestIntegrMapper.cpp \
	TestXMLNode.cpp

