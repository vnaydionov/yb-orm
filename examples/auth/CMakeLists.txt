
cmake_minimum_required (VERSION 2.6)
project (auth)


option (USE_QT4 "YB.ORM built against QT4" OFF)

if (UNIX)
    find_path (BOOST_INCLUDEDIR boost/lexical_cast.hpp /usr/include)
else ()
    find_path (BOOST_INCLUDEDIR boost/lexical_cast.hpp c:/yborm/include)
endif ()

if (NOT USE_QT4)

# find boost + libxml2

if (MSVC)
    set (Boost_USE_STATIC_LIBS ON)
else ()
    set (Boost_USE_STATIC_LIBS OFF)
endif ()
set (Boost_USE_MULTITHREADED ON)
set (Boost_NO_SYSTEM_PATHS ON)
#set (Boost_DEBUG ON)
set (Boost_ADDITIONAL_VERSIONS "1.33.1" "1.38" "1.30.0" "1.46.1")
if (UNIX)
    set (BOOST_LIBRARYDIR "/usr/lib")
else ()
    set (BOOST_LIBRARYDIR "c:/yborm/lib")
endif ()

find_package (Boost 1.33.1 COMPONENTS thread date_time)
if ((NOT Boost_DATE_TIME_LIBRARY) OR (NOT Boost_THREAD_LIBRARY))
    message (FATAL_ERROR "Boost not found")
endif ()

# find includes

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

# find libs

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
else ()
    find_library (LIBXML2_LIBS libxml2 c:/yborm/bin)
endif ()

if (UNIX)
    find_library (ODBC_LIBS odbc)
elseif (BORLAND)
    find_library (ODBC_LIBS odbc32 c:/Borland/BCC55/Lib/PSDK)
elseif (MSVC)
    find_library (ODBC_LIBS odbc32)
elseif (MINGW)
    set (ODBC_LIBS -lodbc32 CACHE STRING "odbc32 lib" FORCE)
endif ()

else (USE_QT4)

find_package (Qt4 4.4.0 REQUIRED QtCore QtXml QtSql)
include (${QT_USE_FILE})
add_definitions (-D YB_USE_QT)

endif ()

if (BOOST_INCLUDEDIR AND (NOT BORLAND))
    add_definitions (-D YB_USE_TUPLE)
endif()

function (check_yborm_root YBORM_ROOT)
    if (EXISTS ${YBORM_ROOT})
        find_path (YBORM_INCLUDES orm/SqlDriver.h ${YBORM_ROOT}/include/yb)
        find_library (YBUTIL_LIB ybutil ${YBORM_ROOT}/lib)
        find_library (YBORM_LIB yborm ${YBORM_ROOT}/lib)
        if (YBORM_INCLUDES AND YBUTIL_LIB AND YBORM_LIB)
	    message (STATUS "YB.ORM found: ${YBORM_ROOT}")
        endif ()
    endif ()
endfunction ()

if (((NOT YBORM_INCLUDES) OR (NOT YBUTIL_LIB) OR (NOT YBORM_LIB))
        AND YBORM_ROOT)
    check_yborm_root (${YBORM_ROOT})
endif ()
if ((NOT YBORM_INCLUDES) OR (NOT YBUTIL_LIB) OR (NOT YBORM_LIB))
    if (UNIX)
        check_yborm_root (/usr/local)
    else ()
        check_yborm_root (c:/yborm)
    endif ()
endif ()
if ((NOT YBORM_INCLUDES) OR (NOT YBUTIL_LIB) OR (NOT YBORM_LIB))
    message(SEND_ERROR "YB.ORM not found")
endif ()

if (MSVC)
    find_library (SOCKET_LIB wsock32)
elseif (BORLAND)
    find_library (SOCKET_LIB wsock32 c:/Borland/BCC55/Lib/PSDK)
elseif (MINGW)
    set (SOCKET_LIB -lwsock32 CACHE STRING "wsock32 lib" FORCE)
endif ()

if (MSVC)
    find_library (UUID_LIB rpcrt4)
elseif (BORLAND)
    find_library (UUID_LIB rpcrt4 c:/Borland/BCC55/Lib/PSDK)
elseif (MINGW)
    set (UUID_LIB -lrpcrt4 CACHE STRING "rpcrt4 lib" FORCE)
endif ()

if (YBUTIL_LIB)
    find_library (YBUTIL_LIB ${YBUTIL_LIB})
endif ()

if (YBORM_LIB)
    find_library (YBORM_LIB ${YBORM_LIB})
endif ()

add_subdirectory (src)

install (FILES README.auth DESTINATION doc)
