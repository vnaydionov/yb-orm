
include_directories (
    ${ICONV_INCLUDES}
    ${LIBXML2_INCLUDES}
    ${SQLITE3_INCLUDEDIR}
    ${SOCI_INCLUDEDIR}
    ${BOOST_INCLUDEDIR}
    ${PROJECT_SOURCE_DIR}/include/yb
    ${PROJECT_SOURCE_DIR}/include/private)

set (SOURCES_CPP
    engine.cpp expression.cpp schema.cpp
    schema_config.cpp xmlizer.cpp sql_driver.cpp
    sql_pool.cpp data_object.cpp domain_object.cpp
    domain_factory.cpp code_gen.cpp)

if (USE_QT4)
    set(SOURCES_CPP ${SOURCES_CPP} qtsql_driver.cpp)
endif ()

if (USE_ODBC)
    set(SOURCES_CPP ${SOURCES_CPP} tiodbc.cpp odbc_driver.cpp)
endif ()

if (USE_SQLITE3)
    set(SOURCES_CPP ${SOURCES_CPP} sqlite_driver.cpp)
endif ()

if (USE_SOCI)
    set(SOURCES_CPP ${SOURCES_CPP} soci_driver.cpp)
endif ()

add_library (yborm ${SOURCES_CPP})

target_link_libraries (
    yborm ybutil
    ${LIBXML2_LIBS} ${YB_BOOST_LIBS} ${ODBC_LIBS}
    ${SQLITE3_LIBS} ${SOCI_LIBS} ${QT_LIBRARIES})

install (TARGETS yborm DESTINATION lib)

