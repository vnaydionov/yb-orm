
cmake_minimum_required (VERSION 2.6)
project (yborm_tutorial)

if (UNIX)
    find_path (ICONV_INCLUDES iconv.h /usr/include)
else ()
    find_path (ICONV_INCLUDES iconv.h c:/yborm/include)
endif ()

if (UNIX)
    find_path (LIBXML2_INCLUDES libxml/xmlwriter.h /usr/include/libxml2)
else ()
    find_path (LIBXML2_INCLUDES libxml/xmlwriter.h c:/yborm/include)
endif ()

if (UNIX)
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp /usr/include)
else ()
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp c:/yborm/include)
endif ()

if (UNIX)
    find_path (YBORM_INCLUDES orm/DataObject.h ${PROJECT_SOURCE_DIR}/../lib)
else ()
    find_path (YBORM_INCLUDES orm/DataObject.h c:/yborm/include)
endif ()

if (MSVC)
    find_library (ICONV_LIBS iconv c:/yborm/lib)
elseif (BORLAND)
    find_library (ICONV_LIBS iconvomf c:/yborm/lib)
endif ()

if (UNIX)
    find_library (LIBXML2_LIBS xml2)
elseif (MSVC)
    find_library (LIBXML2_LIBS libxml2 c:/yborm/lib)
elseif (BORLAND)
    find_library (LIBXML2_LIBS libxml2omf c:/yborm/lib)
endif ()

if (UNIX)
    find_library (BOOST_DATE_TIME_LIBS boost_date_time)
elseif (MSVC)
    find_library (BOOST_DATE_TIME_LIBS libboost_date_time-vc100-mt-gd-1_46_1
            c:/yborm/lib)
elseif (BORLAND)
    find_library (BOOST_DATE_TIME_LIBS boost_date_time-bcb-mt-d-1_33_1
            c:/yborm/lib)
endif ()

if (UNIX)
    find_library (BOOST_THREAD_LIBS boost_thread)
elseif (MSVC)
    find_library (BOOST_THREAD_LIBS libboost_thread-vc100-mt-gd-1_46_1
            c:/yborm/lib)
elseif (BORLAND)
    find_library (BOOST_THREAD_LIBS boost_thread-bcb-mt-d-1_33_1
            c:/yborm/lib)
endif ()

if (UNIX)
    find_library (ODBC_LIBS odbc)
elseif (MSVC)
    find_library (ODBC_LIBS odbc32)
elseif (BORLAND)
    find_library (ODBC_LIBS odbc32 c:/Borland/BCC55/Lib/PSDK)
endif ()

if (UNIX)
    find_library (YBUTIL_LIBS ybutil ${PROJECT_SOURCE_DIR}/../lib/util/.libs)
else ()
    find_library (YBUTIL_LIBS ybutil c:/yborm/lib)
endif ()

if (UNIX)
    find_library (YBORM_LIBS yborm ${PROJECT_SOURCE_DIR}/../lib/orm/.libs)
else ()
    find_library (YBORM_LIBS yborm c:/yborm/lib)
endif ()


add_subdirectory (domain)

include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
	${BOOST_INCLUDES} ${YBORM_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

add_executable (tut1 tut1.cpp ${CMAKE_CURRENT_BINARY_DIR}/domain/Client.cpp)
add_executable (tut2 tut2.cpp ${CMAKE_CURRENT_BINARY_DIR}/domain/Client.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/domain/Order.cpp)

set_source_files_properties (
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Client.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Order.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/tut1.sql
    PROPERTIES GENERATED 1)

add_custom_command (
    OUTPUT tut1.sql
    COMMAND xsltproc --stringparam dialect MYSQL ${PROJECT_SOURCE_DIR}/../src/gen_sql.xsl ${PROJECT_SOURCE_DIR}/tut1.xml > ${CMAKE_CURRENT_BINARY_DIR}/tut1.sql
    DEPENDS ${PROJECT_SOURCE_DIR}/tut1.xml
)

add_custom_target (gen_sql_tut1 DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/tut1.sql)

add_dependencies (tut1 gen_cpp_tut1)
add_dependencies (tut2 gen_cpp_tut1)
add_dependencies (tut1 gen_sql_tut1)
add_dependencies (tut2 gen_sql_tut1)

target_link_libraries (tut1 ${LIBXML2_LIBS} ${BOOST_DATE_TIME_LIBS}
        ${BOOST_THREAD_LIBS} ${ODBC_LIBS} ${YBUTIL_LIBS} ${YBORM_LIBS})

target_link_libraries (tut2 ${LIBXML2_LIBS} ${BOOST_DATE_TIME_LIBS}
        ${BOOST_THREAD_LIBS} ${ODBC_LIBS} ${YBUTIL_LIBS} ${YBORM_LIBS})

