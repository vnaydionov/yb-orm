
add_subdirectory (domain)

include_directories (
    ${ICONV_INCLUDES}
    ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDES}
    ${YBORM_INCLUDES}
    ${PROJECT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR})

add_executable (auth
    auth.cpp tcp_socket.cpp micro_http.cpp logger.cpp md5.c
    ${CMAKE_CURRENT_BINARY_DIR}/domain/User.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/LoginSession.cpp)

set_source_files_properties (
    ${CMAKE_CURRENT_BINARY_DIR}/domain/User.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/LoginSession.cpp
    PROPERTIES GENERATED 1)

add_dependencies (auth generate_domain)

if (UNIX)
target_link_libraries (auth ${YBORM_LIB} ${YBUTIL_LIB} ${LIBXML2_LIBS}
    ${BOOST_DATE_TIME_LIBS} ${BOOST_THREAD_LIBS} ${ODBC_LIBS})
else ()
target_link_libraries (auth ${YBORM_LIB} ${YBUTIL_LIB} ${LIBXML2_LIBS}
    ${BOOST_DATE_TIME_LIBS} ${BOOST_THREAD_LIBS} ${ODBC_LIBS}
    ${SOCKET_LIB} ${UUID_LIB})
endif ()

install (TARGETS auth DESTINATION bin)

