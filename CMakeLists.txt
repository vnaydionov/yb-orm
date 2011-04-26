
cmake_minimum_required (VERSION 2.6)
project (yborm)

# find includes

if (UNIX)
    find_path (ICONV_INCLUDES iconv.h /usr/include)
else ()
    find_path (ICONV_INCLUDES iconv.h
            c:/build-yborm/iconv-1.9.2.win32/include)
endif ()

if (UNIX)
    find_path (LIBXML2_INCLUDES libxml/xmlwriter.h /usr/include/libxml2)
else ()
    find_path (LIBXML2_INCLUDES libxml/xmlwriter.h
            c:/build-yborm/libxml2-2.7.7.win32/include)
endif ()

if (UNIX)
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp /usr/include)
elseif (MSVC)
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp
            c:/build-yborm/boost_1_38_0)
elseif (BORLAND)
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp
            c:/build-yborm/boost_1_33_1)
endif ()

if (UNIX)
    find_path (CPPUNIT_INCLUDES cppunit/TestCase.h /usr/include)
else ()
    find_path (CPPUNIT_INCLUDES cppunit/TestCase.h
            c:/build-yborm/cppunit-1.12.1/include)
endif ()

# find libs

if (MSVC)
    find_library (ICONV_LIBS iconv
            c:/build-yborm/iconv-1.9.2.win32/lib)
elseif (BORLAND)
    find_library (ICONV_LIBS iconvomf
            c:/build-yborm/iconv-1.9.2.win32/lib)
endif ()

if (UNIX)
    find_library (LIBXML2_LIBS xml2)
elseif (MSVC)
    find_library (LIBXML2_LIBS libxml2
            c:/build-yborm/libxml2-2.7.7.win32/lib)
elseif (BORLAND)
    find_library (LIBXML2_LIBS libxml2omf
            c:/build-yborm/libxml2-2.7.7.win32/lib)
endif ()

if (UNIX)
    find_library (BOOST_DATE_TIME_LIBS boost_date_time)
elseif (MSVC)
    find_library (BOOST_DATE_TIME_LIBS libboost_date_time-vc90-mt-gd-1_38
            c:/build-yborm/boost_1_38_0/stage/lib)
elseif (BORLAND)
    find_library (BOOST_DATE_TIME_LIBS boost_date_time-bcb-mt-d-1_33_1
            c:/build-yborm/boost_1_33_1/stage/lib)
endif ()

if (UNIX)
    find_library (BOOST_THREAD_LIBS boost_thread)
elseif (MSVC)
    find_library (BOOST_THREAD_LIBS libboost_thread-vc90-mt-gd-1_38
            c:/build-yborm/boost_1_38_0/stage/lib)
elseif (BORLAND)
    find_library (BOOST_THREAD_LIBS boost_thread-bcb-mt-d-1_33_1
            c:/build-yborm/boost_1_33_1/stage/lib)
endif ()

if (UNIX)
    find_library (CPPUNIT_LIBS cppunit)
elseif (MSVC)
    find_library (CPPUNIT_LIBS cppunitd_dll
            c:/build-yborm/cppunit-1.12.1/lib)
elseif (BORLAND)
    find_library (CPPUNIT_LIBS cppunitd_bc5_dll
            c:/build-yborm/cppunit-1.12.1/lib)
endif ()

if (UNIX)
    find_library (ODBC_LIBS odbc)
elseif (MSVC)
    find_library (ODBC_LIBS odbc32)
elseif (BORLAND)
    find_library (ODBC_LIBS odbc32 c:/Borland/BCC55/Lib/PSDK)
endif ()

add_subdirectory (lib)
add_subdirectory (src)
add_subdirectory (examples)

