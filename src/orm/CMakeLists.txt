
include_directories (
    ${ICONV_INCLUDES}
    ${LIBXML2_INCLUDES}
    ${SQLITE3_INCLUDEDIR}
    ${SOCI_INCLUDEDIR}
    ${BOOST_INCLUDEDIR}
    ${PROJECT_SOURCE_DIR}/include/yb
    ${PROJECT_SOURCE_DIR}/include/private)

set (SOURCES_CPP
    code_gen.cpp
    data_object.cpp
    domain_factory.cpp
    domain_object.cpp
    engine.cpp
    expression.cpp
    schema_config.cpp
    schema.cpp
    schema_reader.cpp
    sql_driver.cpp
    sql_pool.cpp
    xmlizer.cpp
    dialect_sqlite.cpp
)

if (USE_QT)
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

add_library (yborm SHARED 
    ${SOURCES_CPP})

add_definitions (-DYBORM_DLL)

target_link_libraries (
    yborm ybutil
    ${LIBXML2_LIBS} ${YB_BOOST_LIBS} ${ODBC_LIBS}
    ${SQLITE3_LIBS} ${SOCI_LIBS} ${QT_LIBRARIES})

if (UNIX)
    install (TARGETS yborm DESTINATION lib)
else ()
    install (TARGETS yborm
                RUNTIME DESTINATION bin COMPONENT libraries
                LIBRARY DESTINATION lib COMPONENT libraries
                ARCHIVE DESTINATION lib COMPONENT libraries)
endif ()

