
add_subdirectory (test)

include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES}
    ${BOOST_INCLUDEDIR} ${PROJECT_SOURCE_DIR}/lib)

add_library (ybutil String.cpp Exception.cpp Thread.cpp decimal.cpp
    DataTypes.cpp str_utils.cpp xml_writer.cpp xmlnode.cpp nlogger.cpp)

target_link_libraries (ybutil ${LIBXML2_LIBS} ${Boost_DATE_TIME_LIBRARY}
    ${Boost_THREAD_LIBRARY} ${QT_LIBRARIES})

install (TARGETS ybutil DESTINATION lib)
install (FILES Utility.h String.h Exception.h Thread.h decimal.h
    DataTypes.h str_utils.hpp xml_writer.h xmlnode.h nlogger.h
    param.hpp stacktrace.h ResultSet.h Singleton.h
    DESTINATION include/yb/util)

