
cmake_minimum_required (VERSION 2.8)
project (yborm)

option (DEP_LIBS_ROOT "Path where dependency liraries (libxml2, sqlite3, cppunit, etc.) are installed"
        "c:/yborm/dep_libs")
option (BOOST_ROOT "Path where Boost C++ libs are installed" "c:/boost")
option (USE_QT "Build YB.ORM against Qt" OFF)
option (SQLITE3_SRC "Path to SQLite3 amalgamation sources")

if (CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-unused-local-typedefs -Wno-format-security")
endif()

if (UNIX)
    find_path (BOOST_INCLUDEDIR boost/lexical_cast.hpp /usr/include)
else ()
    find_path (BOOST_INCLUDEDIR boost/lexical_cast.hpp ${BOOST_ROOT})
endif ()

if (NOT USE_QT)

# find boost + libxml2

if (MSVC)
    set (Boost_USE_STATIC_LIBS ON)
else ()
    set (Boost_USE_STATIC_LIBS OFF)
endif ()
set (Boost_USE_MULTITHREADED ON)
#set (Boost_NO_SYSTEM_PATHS ON)
#set (Boost_DEBUG ON)
# You may need to explicitly set the following for MSVC 2013:
#set (Boost_COMPILER -vc120)
set (Boost_ADDITIONAL_VERSIONS "1.33.1" "1.38" "1.30.0" "1.46.1")
if (UNIX)
    set (BOOST_LIBRARYDIR "/usr/lib")
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
    find_path (ICONV_INCLUDES iconv.h ${DEP_LIBS_ROOT}/include)
endif ()

if (UNIX)
    find_path (LIBXML2_INCLUDES libxml/xmlwriter.h /usr/include/libxml2)
else ()
    find_path (LIBXML2_INCLUDES libxml/xmlwriter.h ${DEP_LIBS_ROOT}/include)
endif ()

# find libs

if (MSVC)
    find_library (ICONV_LIBS iconv ${DEP_LIBS_ROOT}/lib)
elseif (BORLAND)
    find_library (ICONV_LIBS iconvomf ${DEP_LIBS_ROOT}/lib)
endif ()

if (UNIX)
    find_library (LIBXML2_LIBS xml2)
elseif (MSVC)
    find_library (LIBXML2_LIBS libxml2 ${DEP_LIBS_ROOT}/lib)
elseif (BORLAND)
    find_library (LIBXML2_LIBS libxml2omf ${DEP_LIBS_ROOT}/lib)
else ()
    find_library (LIBXML2_LIBS libxml2 ${DEP_LIBS_ROOT}/bin)
endif ()

else (USE_QT)

string (COMPARE EQUAL "${USE_QT}" "5" USE_QT5)

if (USE_QT5)

    message (STATUS "DEBUG: Qt5 requested")

    cmake_policy (SET CMP0028 NEW)
    cmake_policy (SET CMP0020 NEW)

    find_package (Qt5 5.0.2 REQUIRED Core Xml Sql)

    message (STATUS
        "DEBUG: Qt5Core_INCLUDE_DIRS=${Qt5Core_INCLUDE_DIRS}")
    message (STATUS
        "DEBUG: Qt5Core_LIBRARIES=${Qt5Core_LIBRARIES}")
    message (STATUS
        "DEBUG: Qt5Core_EXECUTABLE_COMPILE_FLAGS=${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

    include_directories (
        ${Qt5Core_INCLUDE_DIRS} ${Qt5Xml_INCLUDE_DIRS}
        ${Qt5Sql_INCLUDE_DIRS})

    set (QT_LIBRARIES ${Qt5Core_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5Sql_LIBRARIES})

    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

else (USE_QT5)

    message (STATUS "DEBUG: Qt4 assumed")

    find_package (Qt4 4.4.0 REQUIRED QtCore QtXml QtSql)

    include (${QT_USE_FILE})

endif ()

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

if (SQLITE3_SRC)
    find_path (SQLITE3_INCLUDEDIR sqlite3.h ${SQLITE3_SRC})
    find_path (SQLITE3_SINGLE_SOURCE sqlite3.c ${SQLITE3_SRC})
    set (SQLITE3_LIBS "")
else ()

if (SQLITE3_INCLUDEDIR)
    find_path (SQLITE3_INCLUDEDIR sqlite3.h ${SQLITE3_INCLUDEDIR})
elseif (UNIX)
    find_path (SQLITE3_INCLUDEDIR sqlite3.h /usr/include)
else ()
    find_path (SQLITE3_INCLUDEDIR sqlite3.h ${DEP_LIBS_ROOT}/include)
endif ()

if (SQLITE3_LIBDIR)
    find_library (SQLITE3_LIBS sqlite3 ${SQLITE3_LIBDIR})
elseif (UNIX)
    find_library (SQLITE3_LIBS sqlite3)
elseif (BORLAND)
    find_library (SQLITE3_LIBS sqlite3omf ${DEP_LIBS_ROOT}/lib)
else ()
    find_library (SQLITE3_LIBS sqlite3 ${DEP_LIBS_ROOT}/lib ${DEP_LIBS_ROOT}/bin)
endif ()

endif ()

if (SQLITE3_INCLUDEDIR AND (SQLITE3_LIBS OR SQLITE3_SINGLE_SOURCE))
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
    find_path (SOCI_INCLUDEDIR soci.h ${DEP_LIBS_ROOT}/include)
endif ()

if (SOCI_LIBDIR)
    find_library (SOCI_LIBS soci_core ${SOCI_LIBDIR})
elseif (UNIX)
    find_library (SOCI_LIBS soci_core)
else ()
    find_library (SOCI_LIBS soci ${DEP_LIBS_ROOT}/lib ${DEP_LIBS_ROOT}/bin)
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
    find_path (CPPUNIT_INCLUDES cppunit/TestCase.h ${DEP_LIBS_ROOT}/include)
endif ()

if (UNIX)
    find_library (CPPUNIT_LIBS cppunit)
elseif (MSVC)
    find_library (CPPUNIT_LIBS cppunitd_dll ${DEP_LIBS_ROOT}/lib)
elseif (BORLAND)
    find_library (CPPUNIT_LIBS cppunitd_bc5_dll ${DEP_LIBS_ROOT}/lib)
else ()
    find_library (CPPUNIT_LIBS cppunit ${DEP_LIBS_ROOT}/lib)
endif ()

install (FILES
    CHANGES COPYING 
    DESTINATION doc)

add_subdirectory (include)
add_subdirectory (src)
add_subdirectory (tests)
add_subdirectory (examples)
add_subdirectory (doc)

