
add_subdirectory (test)

include_directories (${ICONV_INCLUDES}
    ${LIBXML2_INCLUDES}
    ${SQLITE3_INCLUDEDIR}
    ${SOCI_INCLUDEDIR}
    ${BOOST_INCLUDEDIR}
    ${PROJECT_SOURCE_DIR}/lib)

set (SOURCES_CPP Engine.cpp Expression.cpp MetaData.cpp
    XMLMetaDataConfig.cpp XMLizer.cpp SqlDriver.cpp
    SqlPool.cpp DataObject.cpp DomainObject.cpp DomainFactory.cpp
    CodeGen.cpp)

set (SOURCES_H Expression.h
    SqlDriver.h SqlPool.h XMLizer.h
    DomainObject.h DomainFactory.h Engine.h MetaData.h
    XMLMetaDataConfig.h DataObject.h CodeGen.h)

if (USE_QT4)
    set(SOURCES_CPP ${SOURCES_CPP} QtSqlDriver.cpp)
    set(SOURCES_H ${SOURCES_H} QtSqlDriver.h)
else ()
    set(SOURCES_CPP ${SOURCES_CPP} tiodbc.cpp OdbcDriver.cpp)
    set(SOURCES_H ${SOURCES_H} tiodbc.hpp OdbcDriver.h)
    if (USE_SQLITE3)
        set(SOURCES_CPP ${SOURCES_CPP} SQLiteDriver.cpp)
        set(SOURCES_H ${SOURCES_H} SQLiteDriver.h)
    endif ()
    if (USE_SOCI)
        set(SOURCES_CPP ${SOURCES_CPP} SOCIDriver.cpp)
        set(SOURCES_H ${SOURCES_H} SOCIDriver.h)
    endif ()
endif ()

add_library (yborm ${SOURCES_CPP})

target_link_libraries (yborm ybutil ${LIBXML2_LIBS}
    ${YB_BOOST_LIBS} ${ODBC_LIBS} ${SQLITE3_LIBS} ${SOCI_LIBS} ${QT_LIBRARIES})

install (TARGETS yborm DESTINATION lib)
install (FILES ${SOURCES_H} DESTINATION include/yb/orm)

