
include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDEDIR} ${PROJECT_SOURCE_DIR}/lib)

add_executable (yborm_gen yborm_gen.cpp)

target_link_libraries (yborm_gen yborm ybutil ${LIBXML2_LIBS}
    ${YB_BOOST_LIBS} ${ODBC_LIBS} ${SQLITE3_LIBS} ${SOCI_LIBS}
    ${QT_LIBRARIES})

install (TARGETS yborm_gen DESTINATION bin)
install (FILES yborm_gen_sql.bat init_sqlite.bat DESTINATION examples)

