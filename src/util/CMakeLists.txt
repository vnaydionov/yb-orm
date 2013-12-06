
include_directories (
    ${ICONV_INCLUDES} ${LIBXML2_INCLUDES} ${BOOST_INCLUDEDIR}
    ${PROJECT_SOURCE_DIR}/include/yb
    ${PROJECT_SOURCE_DIR}/include/private)

add_library (ybutil
    string_type.cpp exception.cpp thread.cpp decimal.cpp
    data_types.cpp string_utils.cpp xml_writer.cpp
    element_tree.cpp value_type.cpp nlogger.cpp)

target_link_libraries (ybutil
    ${LIBXML2_LIBS} ${YB_BOOST_LIBS} ${QT_LIBRARIES})

install (TARGETS ybutil DESTINATION lib)

