
include_directories (
    ${ICONV_INCLUDES} ${LIBXML2_INCLUDES} ${BOOST_INCLUDEDIR}
    ${PROJECT_SOURCE_DIR}/include/yb
    ${PROJECT_SOURCE_DIR}/include/private)

add_definitions (-D YBUTIL_DLL)

add_library (ybutil SHARED
    data_types.cpp
    decimal.cpp
    element_tree.cpp
    exception.cpp
    nlogger.cpp
    string_type.cpp
    string_utils.cpp
    thread.cpp
    utility.cpp
    value_type.cpp
    xml_writer.cpp
)

target_link_libraries (ybutil
    ${LIBXML2_LIBS} ${YB_BOOST_LIBS} ${QT_LIBRARIES})

if (UNIX)
    install (TARGETS ybutil DESTINATION lib)
else ()
    install (TARGETS ybutil
                RUNTIME DESTINATION bin COMPONENT libraries
                LIBRARY DESTINATION lib COMPONENT libraries
                ARCHIVE DESTINATION lib COMPONENT libraries)
endif ()

