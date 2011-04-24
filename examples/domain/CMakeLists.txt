
add_custom_command (
    OUTPUT Client.cpp Order.cpp
    COMMAND orm_domain_generator ${PROJECT_SOURCE_DIR}/examples/ex1_schema.xml
        ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS orm_domain_generator ${PROJECT_SOURCE_DIR}/examples/ex1_schema.xml
)

add_custom_target (generate_domain DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/Client.cpp)

