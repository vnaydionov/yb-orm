
#add_definitions(-D_UNICODE -DUNICODE)

cmake_minimum_required (VERSION 2.6)
project (auth)

# find includes

function (check_yborm_root YBORM_ROOT)
    if (EXISTS ${YBORM_ROOT})
        find_path (YBORM_INCLUDES orm/SqlDriver.h ${YBORM_ROOT}/include/yb)
        find_library (YBUTIL_LIB ybutil ${YBORM_ROOT}/lib)
        find_library (YBORM_LIB yborm ${YBORM_ROOT}/lib)
        if ((NOT DEFINED YBORM_INCLUDES) OR (NOT DEFINED YBUTIL_LIB)
                                         OR (NOT DEFINED YBORM_LIB))
            unset(YBORM_INCLUDES)
            unset(YBUTIL_LIB)
            unset(YBORM_LIB)
        endif ()
    endif ()
endfunction ()

if ((NOT DEFINED YBORM_INCLUDES) AND (NOT DEFINED YBUTIL_LIB)
                                 AND (NOT DEFINED YBORM_LIB))
    if (DEFINED YBORM_ROOT)
        check_yborm_root (${YBORM_ROOT})
    endif ()
    if (NOT DEFINED YBORM_INCLUDES)
        if (UNIX)
            check_yborm_root (/usr/local)
        else ()
            check_yborm_root (c:\yborm)
        endif ()
    endif ()
endif ()

if (UNIX)
    find_path (ICONV_INCLUDES iconv.h /usr/include)
else ()
    find_path (ICONV_INCLUDES iconv.h
            c:/yborm/include)
endif ()

if (UNIX)
    find_path (LIBXML2_INCLUDES libxml/xmlwriter.h /usr/include/libxml2)
else ()
    find_path (LIBXML2_INCLUDES libxml/xmlwriter.h
            c:/yborm/include)
endif ()

if (UNIX)
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp /usr/include)
elseif (MSVC)
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp
            c:/yborm/include)
elseif (BORLAND)
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp
            c:/yborm/include)
endif ()

if (DEFINED YBORM_INCLUDES)
    find_path (YBORM_INCLUDES orm/SqlDriver.h ${YBORM_INCLUDES})
endif ()

# find libs

if (MSVC)
    find_library (ICONV_LIBS iconv
            c:/yborm/lib)
elseif (BORLAND)
    find_library (ICONV_LIBS iconvomf
            c:/yborm/lib)
endif ()

if (UNIX)
    find_library (LIBXML2_LIBS xml2)
elseif (MSVC)
    find_library (LIBXML2_LIBS libxml2
            c:/yborm/lib)
elseif (BORLAND)
    find_library (LIBXML2_LIBS libxml2omf
            c:/yborm/lib)
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

if (MSVC)
    find_library (SOCKET_LIB wsock32)
elseif (BORLAND)
    find_library (SOCKET_LIB wsock32 c:/Borland/BCC55/Lib/PSDK)
endif ()

if (MSVC)
    find_library (UUID_LIB rpcrt4)
elseif (BORLAND)
    find_library (UUID_LIB rpcrt4 c:/Borland/BCC55/Lib/PSDK)
endif ()

if (DEFINED YBUTIL_LIB)
    find_library (YBUTIL_LIB ${YBUTIL_LIB})
endif ()

if (DEFINED YBORM_LIB)
    find_library (YBORM_LIB ${YBORM_LIB})
endif ()

add_subdirectory (src)

