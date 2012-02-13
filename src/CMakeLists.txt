
include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES} ${BOOST_INCLUDES}
    ${PROJECT_SOURCE_DIR}/lib)

add_executable (yborm_gen_domain yborm_gen_domain.cpp)

target_link_libraries (yborm_gen_domain yborm ybutil ${LIBXML2_LIBS}
    ${BOOST_DATE_TIME_LIBS} ${BOOST_THREAD_LIBS} ${ODBC_LIBS})

install (TARGETS yborm_gen_domain DESTINATION bin)

