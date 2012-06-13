
cmake_minimum_required (VERSION 2.6)
project (yborm)


option (USE_QT4 "Build YB.ORM against QT4" OFF)

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
    find_path (BOOST_INCLUDEDIR boost/lexical_cast.hpp /usr/include)
else ()
    find_path (BOOST_INCLUDEDIR boost/lexical_cast.hpp c:/yborm/include)
endif ()
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

else (USE_QT4)

find_package (Qt4 4.4.0 REQUIRED QtCore QtGui QtXml)
include (${QT_USE_FILE})
add_definitions (-D YB_USE_QT)

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

add_subdirectory (lib)
add_subdirectory (src)
add_subdirectory (examples)

