
add_custom_command (
    OUTPUT Client.cpp Order.cpp OrderItem.cpp Product.cpp ProductGroup.cpp 
           Contract.cpp
    COMMAND yborm_gen_domain ${PROJECT_SOURCE_DIR}/examples/ex2_schema.xml
        ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS yborm_gen_domain ${PROJECT_SOURCE_DIR}/examples/ex2_schema.xml
)

add_custom_target (generate_domain DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/Client.cpp)

