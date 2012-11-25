
add_subdirectory (test)

include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDEDIR} ${PROJECT_SOURCE_DIR}/lib)

add_library (ybutil String.cpp Exception.cpp Thread.cpp decimal.cpp
    DataTypes.cpp str_utils.cpp XmlWriter.cpp ElementTree.cpp nlogger.cpp)

target_link_libraries (ybutil ${LIBXML2_LIBS}
    ${YB_BOOST_LIBS} ${QT_LIBRARIES})

install (TARGETS ybutil DESTINATION lib)
install (FILES Utility.h String.h Exception.h Thread.h decimal.h
    DataTypes.h str_utils.hpp XmlWriter.h ElementTree.h nlogger.h
    param.hpp stacktrace.h ResultSet.h Singleton.h
    DESTINATION include/yb/util)

