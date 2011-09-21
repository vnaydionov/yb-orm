 
include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES} ${BOOST_INCLUDES}
    ${CPPUNIT_INCLUDES} ${PROJECT_SOURCE_DIR}/lib)

add_executable (ybutil_unit_tests
    TestDecimal.cpp TestSingleton.cpp TestResultSet.cpp
	TestMisc.cpp)

target_link_libraries (ybutil_unit_tests testmain ybutil ${LIBXML2_LIBS}
    ${BOOST_DATE_TIME_LIBS} ${BOOST_THREAD_LIBS} ${CPPUNIT_LIBS})

add_test (ybutil_unit_tests ybutil_unit_tests)

