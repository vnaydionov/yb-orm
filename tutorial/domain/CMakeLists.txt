
add_custom_command (
    OUTPUT Client.cpp Order.cpp
    COMMAND ${YBORM_ROOT}/bin/yborm_gen --domain ${PROJECT_SOURCE_DIR}/tut1.xml
        ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${PROJECT_SOURCE_DIR}/tut1.xml
)

add_custom_target (gen_cpp_tut1 DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/Client.cpp)

add_custom_command (
    OUTPUT ProductGroup.cpp Product.cpp
    COMMAND ${YBORM_ROOT}/bin/yborm_gen --domain ${PROJECT_SOURCE_DIR}/tut4.xml
        ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${PROJECT_SOURCE_DIR}/tut4.xml
)

add_custom_target (gen_cpp_tut4 DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/ProductGroup.cpp)

