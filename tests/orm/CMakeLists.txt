
include_directories (
    ${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDEDIR} ${CPPUNIT_INCLUDES}
    ${PROJECT_SOURCE_DIR}/include/yb)

add_executable (yborm_unit_tests
    test_engine.cpp test_expression.cpp test_schema.cpp
    test_schema_config.cpp test_xmlizer.cpp test_data_object.cpp)

target_link_libraries (yborm_unit_tests
    testmain ybutil yborm
    ${LIBXML2_LIBS} ${YB_BOOST_LIBS}
    ${ODBC_LIBS} ${SQLITE3_LIBS} ${SOCI_LIBS}
    ${CPPUNIT_LIBS} ${QT_LIBRARIES})

add_test (yborm_unit_tests yborm_unit_tests)

install (TARGETS yborm_unit_tests DESTINATION examples)
install (FILES yborm_unit_tests.bat testdb_schema.xml DESTINATION examples)
