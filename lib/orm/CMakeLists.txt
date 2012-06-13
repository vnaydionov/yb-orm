
add_subdirectory (test)

include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDEDIR} ${PROJECT_SOURCE_DIR}/lib)

add_library (yborm Engine.cpp Filters.cpp MetaData.cpp
    tiodbc.cpp Value.cpp XMLMetaDataConfig.cpp XMLNode.cpp SqlDriver.cpp
    OdbcDriver.cpp SqlPool.cpp DataObject.cpp DomainObj.cpp DomainFactory.cpp)

target_link_libraries (yborm ybutil ${LIBXML2_LIBS}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} ${ODBC_LIBS}
    ${QT_LIBRARIES})

install (TARGETS yborm DESTINATION lib)
install (FILES DomainFactorySingleton.h Filters.h
    MetaDataSingleton.h SqlDriver.h OdbcDriver.h SqlPool.h Value.h XMLNode.h
    DomainObj.h DomainFactory.h Engine.h MetaData.h
    tiodbc.hpp XMLMetaDataConfig.h DataObject.h
    DESTINATION include/yb/orm)

