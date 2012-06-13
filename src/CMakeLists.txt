
include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDEDIR} ${PROJECT_SOURCE_DIR}/lib)

add_executable (yborm_gen_domain yborm_gen_domain.cpp)

target_link_libraries (yborm_gen_domain yborm ybutil ${LIBXML2_LIBS}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} ${ODBC_LIBS}
    ${QT_LIBRARIES})

install (TARGETS yborm_gen_domain DESTINATION bin)
install (FILES yborm_gen_sql.bat yborm_gen_sql.xsl DESTINATION examples)

