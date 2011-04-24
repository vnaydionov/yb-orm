
include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES} ${BOOST_INCLUDES}
    ${PROJECT_SOURCE_DIR}/lib)

add_executable (orm_domain_generator orm_domain_generator.cpp)

target_link_libraries (orm_domain_generator yborm ybutil ${LIBXML2_LIBS}
    ${BOOST_DATE_TIME_LIBS} ${BOOST_THREAD_LIBS} ${ODBC_LIBS})

install (TARGETS orm_domain_generator DESTINATION bin)

