
add_custom_command (
    OUTPUT Client.cpp Order.cpp
    COMMAND ${PROJECT_SOURCE_DIR}/../src/yborm_gen_domain ${PROJECT_SOURCE_DIR}/tut1.xml
        ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${PROJECT_SOURCE_DIR}/tut1.xml
)

add_custom_target (gen_cpp_tut1 DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/Client.cpp)

