
add_subdirectory (test)

include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES} ${BOOST_INCLUDES})

add_library (ybutil decimal.cpp str_utils.cpp xmlnode.cpp xml_writer.cpp)

target_link_libraries (ybutil ${LIBXML2_LIBS} ${BOOST_DATE_TIME_LIBS}
    ${BOOST_THREAD_LIBS})

install (TARGETS ybutil DESTINATION lib)
install (FILES decimal.h str_utils.hpp xmlnode.h xml_writer.h param.hpp
    Singleton.h
             DESTINATION include/yb/util)

