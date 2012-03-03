
add_subdirectory (test)

include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES} ${BOOST_INCLUDES}
    ${PROJECT_SOURCE_DIR}/lib)

add_library (yborm Engine.cpp Filters.cpp MetaData.cpp
    tiodbc.cpp Value.cpp XMLMetaDataConfig.cpp XMLNode.cpp SqlDriver.cpp
    DataObject.cpp DomainObj.cpp DomainFactory.cpp)

target_link_libraries (yborm ybutil ${LIBXML2_LIBS}
    ${BOOST_DATE_TIME_LIBS} ${BOOST_THREAD_LIBS} ${ODBC_LIBS})

install (TARGETS yborm DESTINATION lib)
install (FILES DomainFactorySingleton.h Filters.h
    MetaDataSingleton.h SqlDriver.h Value.h XMLNode.h
    DomainObj.h DomainFactory.h Engine.h MetaData.h
    tiodbc.hpp XMLMetaDataConfig.h DataObject.h
             DESTINATION include/yb/orm)

