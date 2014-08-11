
add_custom_command (
    OUTPUT Client.cpp Order.cpp OrderItem.cpp Product.cpp ProductGroup.cpp 
           Contract.cpp
    COMMAND ${CMAKE_COMMAND} -DSDIR=${PROJECT_SOURCE_DIR} -DBDIR=${CMAKE_BINARY_DIR} -P ${PROJECT_SOURCE_DIR}/examples/domain/gen.cmake
    DEPENDS yborm_gen ${PROJECT_SOURCE_DIR}/examples/ex2_schema.xml
)

add_custom_target (generate_domain DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/Client.cpp)

