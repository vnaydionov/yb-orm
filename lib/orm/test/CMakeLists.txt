
include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDEDIR} ${CPPUNIT_INCLUDES} ${PROJECT_SOURCE_DIR}/lib)

add_executable (yborm_unit_tests
    TestEngine.cpp TestFilters.cpp
    TestMetaData.cpp TestValue.cpp
    TestXMLConfig.cpp TestXMLNode.cpp TestDataObject.cpp)

target_link_libraries (yborm_unit_tests testmain ybutil yborm
    ${LIBXML2_LIBS} ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY}
    ${ODBC_LIBS} ${SQLITE3_LIBS} ${CPPUNIT_LIBS} ${QT_LIBRARIES})

add_test (yborm_unit_tests yborm_unit_tests)

install (TARGETS yborm_unit_tests DESTINATION examples)
install (FILES yborm_unit_tests.bat test_schema.xml DESTINATION examples)
