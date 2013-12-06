 
include_directories (
    ${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDEDIR} ${CPPUNIT_INCLUDES}
    ${PROJECT_SOURCE_DIR}/include/yb)

add_executable (ybutil_unit_tests
    test_decimal.cpp test_singleton.cpp
    test_result_set.cpp test_misc.cpp
    test_value_type.cpp)

target_link_libraries (ybutil_unit_tests
    testmain ybutil
    ${LIBXML2_LIBS} ${YB_BOOST_LIBS}
    ${CPPUNIT_LIBS} ${QT_LIBRARIES})

add_test (ybutil_unit_tests ybutil_unit_tests)

install (TARGETS ybutil_unit_tests DESTINATION examples)
install (FILES ybutil_unit_tests.bat DESTINATION examples)

