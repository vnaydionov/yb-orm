 
include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDEDIR} ${CPPUNIT_INCLUDES} ${PROJECT_SOURCE_DIR}/lib)

add_executable (ybutil_unit_tests
    TestDecimal.cpp TestSingleton.cpp TestResultSet.cpp TestMisc.cpp
    TestValue.cpp)

target_link_libraries (ybutil_unit_tests testmain ybutil ${LIBXML2_LIBS}
    ${YB_BOOST_LIBS} ${CPPUNIT_LIBS} ${QT_LIBRARIES})

add_test (ybutil_unit_tests ybutil_unit_tests)

install (TARGETS ybutil_unit_tests DESTINATION examples)
install (FILES ybutil_unit_tests.bat DESTINATION examples)

