
cmake_minimum_required (VERSION 2.6)
project (yborm)

# find includes

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
else ()
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp
            c:/yborm/include)
endif ()

if (UNIX)
    find_path (CPPUNIT_INCLUDES cppunit/TestCase.h /usr/include)
else ()
    find_path (CPPUNIT_INCLUDES cppunit/TestCase.h
            c:/yborm/include)
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
else ()
    find_library (LIBXML2_LIBS libxml2
            c:/yborm/bin)
endif ()

if (UNIX)
    find_library (BOOST_DATE_TIME_LIBS boost_date_time)
elseif (MSVC)
    find_library (BOOST_DATE_TIME_LIBS libboost_date_time-vc100-mt-gd-1_46_1
            c:/yborm/lib)
elseif (BORLAND)
    find_library (BOOST_DATE_TIME_LIBS boost_date_time-bcb-mt-d-1_33_1
            c:/yborm/lib)
elseif (MINGW)
    find_library (BOOST_DATE_TIME_LIBS libboost_date_time-mgw44-mt-d-1_46_1.dll
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
elseif (MINGW)
    find_library (BOOST_THREAD_LIBS libboost_thread-mgw44-mt-d-1_46_1.dll
            c:/yborm/lib)
endif ()

if (UNIX)
    find_library (CPPUNIT_LIBS cppunit)
elseif (MSVC)
    find_library (CPPUNIT_LIBS cppunitd_dll
            c:/yborm/lib)
elseif (BORLAND)
    find_library (CPPUNIT_LIBS cppunitd_bc5_dll
            c:/yborm/lib)
else ()
    find_library (CPPUNIT_LIBS cppunit
            c:/yborm/lib)
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

add_subdirectory (lib)
add_subdirectory (src)
add_subdirectory (examples)
add_subdirectory (doc)

install (FILES AUTHORS BUILD.win32 CHANGES COPYING README README.rus TODO
    DESTINATION doc)

