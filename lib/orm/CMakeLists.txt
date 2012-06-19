
add_subdirectory (test)

include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDEDIR} ${PROJECT_SOURCE_DIR}/lib)

set (SOURCES_CPP Engine.cpp Filters.cpp MetaData.cpp
    Value.cpp XMLMetaDataConfig.cpp XMLNode.cpp SqlDriver.cpp
    SqlPool.cpp DataObject.cpp DomainObj.cpp DomainFactory.cpp)

set (SOURCES_H DomainFactorySingleton.h Filters.h
    MetaDataSingleton.h SqlDriver.h SqlPool.h Value.h XMLNode.h
    DomainObj.h DomainFactory.h Engine.h MetaData.h
    XMLMetaDataConfig.h DataObject.h)

if (NOT USE_QT4)
    set(SOURCES_CPP ${SOURCES_CPP} tiodbc.cpp OdbcDriver.cpp)
    set(SOURCES_H ${SOURCES_H} tiodbc.hpp OdbcDriver.h)
else ()
    set(SOURCES_CPP ${SOURCES_CPP} QtSqlDriver.cpp)
    set(SOURCES_H ${SOURCES_H} QtSqlDriver.h)
endif ()

add_library (yborm ${SOURCES_CPP})

target_link_libraries (yborm ybutil ${LIBXML2_LIBS}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} ${ODBC_LIBS}
    ${QT_LIBRARIES})

install (TARGETS yborm DESTINATION lib)
install (FILES ${SOURCES_H} DESTINATION include/yb/orm)

