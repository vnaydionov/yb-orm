
cmake_minimum_required (VERSION 2.8)
project (yborm)

option (USE_QT "Build YB.ORM against Qt" OFF)

if (UNIX)
    find_path (BOOST_INCLUDEDIR boost/lexical_cast.hpp /usr/include)
else ()
    find_path (BOOST_INCLUDEDIR boost/lexical_cast.hpp c:/yborm/include)
endif ()

if (NOT USE_QT)

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

find_package (Boost 1.33.1 COMPONENTS system thread date_time chrono)
if ((NOT Boost_DATE_TIME_LIBRARY) OR (NOT Boost_THREAD_LIBRARY))
    message (FATAL_ERROR "Boost not found")
endif ()
if (Boost_SYSTEM_LIBRARY)
    set (YB_BOOST_LIBS ${Boost_SYSTEM_LIBRARY})
endif ()
if (Boost_CHRONO_LIBRARY)
    set (YB_BOOST_LIBS ${YB_BOOST_LIBS} ${Boost_CHRONO_LIBRARY})
endif ()
set (YB_BOOST_LIBS ${YB_BOOST_LIBS} ${Boost_THREAD_LIBRARY}
                   ${Boost_DATE_TIME_LIBRARY})

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

else (USE_QT)

find_package (Qt4 4.4.0 REQUIRED QtCore QtXml QtSql)
#find_package (Qt5 5.0.2 REQUIRED QtCore QtXml QtSql)
include (${QT_USE_FILE})
add_definitions (-DYB_USE_QT)

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
add_definitions (-DYB_USE_ODBC)
set (USE_ODBC ON)

if (SQLITE3_INCLUDEDIR)
    find_path (SQLITE3_INCLUDEDIR sqlite3.h ${SQLITE3_INCLUDEDIR})
elseif (UNIX)
    find_path (SQLITE3_INCLUDEDIR sqlite3.h /usr/include)
else ()
    find_path (SQLITE3_INCLUDEDIR sqlite3.h c:/yborm/include)
endif ()

if (SQLITE3_LIBDIR)
    find_library (SQLITE3_LIBS sqlite3 ${SQLITE3_LIBDIR})
elseif (UNIX)
    find_library (SQLITE3_LIBS sqlite3)
elseif (BORLAND)
    find_library (SQLITE3_LIBS sqlite3omf)
else ()
    find_library (SQLITE3_LIBS sqlite3 c:/yborm/lib c:/yborm/bin)
endif ()

if (SQLITE3_INCLUDEDIR AND SQLITE3_LIBS)
    add_definitions (-DYB_USE_SQLITE3)
    set (USE_SQLITE3 ON)
else ()
    set (SQLITE3_LIBS "")
    set (SQLITE3_INCLUDEDIR "")
    set (USE_SQLITE3 OFF)
endif ()

if (SOCI_INCLUDEDIR)
    find_path (SOCI_INCLUDEDIR soci.h ${SOCI_INCLUDEDIR})
elseif (UNIX)
    find_path (SOCI_INCLUDEDIR soci.h /usr/include/soci /usr/local/include/soci)
else ()
    find_path (SOCI_INCLUDEDIR soci.h c:/yborm/include)
endif ()

if (SOCI_LIBDIR)
    find_library (SOCI_LIBS soci_core ${SOCI_LIBDIR})
elseif (UNIX)
    find_library (SOCI_LIBS soci_core)
else ()
    find_library (SOCI_LIBS soci c:/yborm/lib c:/yborm/bin)
endif ()

if (SOCI_INCLUDEDIR AND SOCI_LIBS)
    add_definitions (-DYB_USE_SOCI)
    set (USE_SOCI ON)
else ()
    set (SOCI_LIBS "")
    set (SOCI_INCLUDEDIR "")
    set (USE_SOCI OFF)
endif ()

if (BOOST_INCLUDEDIR AND (NOT BORLAND))
    add_definitions (-DYB_USE_TUPLE)
endif()

if (UNIX)
    find_path (CPPUNIT_INCLUDES cppunit/TestCase.h /usr/include)
else ()
    find_path (CPPUNIT_INCLUDES cppunit/TestCase.h c:/yborm/include)
endif ()

if (UNIX)
    find_library (CPPUNIT_LIBS cppunit)
elseif (MSVC)
    find_library (CPPUNIT_LIBS cppunitd_dll c:/yborm/lib)
elseif (BORLAND)
    find_library (CPPUNIT_LIBS cppunitd_bc5_dll c:/yborm/lib)
else ()
    find_library (CPPUNIT_LIBS cppunit c:/yborm/lib)
endif ()

install (FILES
    CHANGES COPYING 
    DESTINATION doc)

add_subdirectory (include)
add_subdirectory (src)
add_subdirectory (tests)
add_subdirectory (examples)
add_subdirectory (doc)

