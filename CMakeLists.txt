
cmake_minimum_required (VERSION 2.6)
project (yborm)

add_definitions(-D_UNICODE -DUNICODE)

# find includes

if (UNIX)
    find_path (ICONV_INCLUDES iconv.h /usr/include)
else ()
    find_path (ICONV_INCLUDES iconv.h
            c:/work/build-yborm/iconv-1.9.2.win32/include)
endif ()

if (UNIX)
    find_path (LIBXML2_INCLUDES libxml/xmlwriter.h /usr/include/libxml2)
else ()
    find_path (LIBXML2_INCLUDES libxml/xmlwriter.h
            c:/work/build-yborm/libxml2-2.7.7.win32/include)
endif ()

if (UNIX)
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp /usr/include)
elseif (MSVC)
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp
            c:/work/build-yborm/boost_1_46_1)
elseif (BORLAND)
    find_path (BOOST_INCLUDES boost/lexical_cast.hpp
            c:/work/build-yborm/boost_1_33_1)
endif ()

if (UNIX)
    find_path (CPPUNIT_INCLUDES cppunit/TestCase.h /usr/include)
else ()
    find_path (CPPUNIT_INCLUDES cppunit/TestCase.h
            c:/work/build-yborm/cppunit-1.12.1/include)
endif ()

# find libs

if (MSVC)
    find_library (ICONV_LIBS iconv
            c:/work/build-yborm/iconv-1.9.2.win32/lib)
elseif (BORLAND)
    find_library (ICONV_LIBS iconvomf
            c:/work/build-yborm/iconv-1.9.2.win32/lib)
endif ()

if (UNIX)
    find_library (LIBXML2_LIBS xml2)
elseif (MSVC)
    find_library (LIBXML2_LIBS libxml2
            c:/work/build-yborm/libxml2-2.7.7.win32/lib)
elseif (BORLAND)
    find_library (LIBXML2_LIBS libxml2omf
            c:/work/build-yborm/libxml2-2.7.7.win32/lib)
endif ()

if (UNIX)
    find_library (BOOST_DATE_TIME_LIBS boost_date_time)
elseif (MSVC)
    find_library (BOOST_DATE_TIME_LIBS libboost_date_time-vc100-mt-gd-1_46_1
            c:/work/build-yborm/boost_1_46_1/stage/lib)
elseif (BORLAND)
    find_library (BOOST_DATE_TIME_LIBS boost_date_time-bcb-d-1_33_1
            c:/work/build-yborm/boost_1_33_1/stage/lib)
endif ()

if (UNIX)
    find_library (BOOST_THREAD_LIBS boost_thread)
elseif (MSVC)
    find_library (BOOST_THREAD_LIBS libboost_thread-vc100-mt-gd-1_46_1
            c:/work/build-yborm/boost_1_46_1/stage/lib)
elseif (BORLAND)
    find_library (BOOST_THREAD_LIBS boost_thread-bcb-mt-d-1_33_1
            c:/work/build-yborm/boost_1_33_1/stage/lib)
endif ()

if (UNIX)
    find_library (CPPUNIT_LIBS cppunit)
elseif (MSVC)
    find_library (CPPUNIT_LIBS cppunitd_dll
            c:/work/build-yborm/cppunit-svn/lib)
elseif (BORLAND)
    find_library (CPPUNIT_LIBS cppunitd_bc5_dll
            c:/work/build-yborm/cppunit-1.12.1/bcc)
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

