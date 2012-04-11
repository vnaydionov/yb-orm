
add_subdirectory (domain)

include_directories (${ICONV_INCLUDES} ${LIBXML2_INCLUDES} ${BOOST_INCLUDES}
    ${PROJECT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR})

add_executable (example1 example1.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Client.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Order.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/OrderItem.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Product.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/ProductGroup.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Contract.cpp)

add_executable (example2 example2.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Client.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Order.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/OrderItem.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Product.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/ProductGroup.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Contract.cpp)

set_source_files_properties (
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Client.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/domain/Order.cpp
    PROPERTIES GENERATED 1)

add_dependencies (example1 generate_domain)
add_dependencies (example2 generate_domain)

target_link_libraries (example1 ybutil yborm ${LIBXML2_LIBS}
    ${BOOST_DATE_TIME_LIBS} ${BOOST_THREAD_LIBS} ${ODBC_LIBS})

target_link_libraries (example2 ybutil yborm ${LIBXML2_LIBS}
    ${BOOST_DATE_TIME_LIBS} ${BOOST_THREAD_LIBS} ${ODBC_LIBS})

install (TARGETS example1 example2 DESTINATION examples)
install (FILES ex1_schema.xml ex2_schema.xml 
	example1.bat example2.bat DESTINATION examples)


