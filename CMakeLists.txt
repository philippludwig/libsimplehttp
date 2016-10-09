project(simplehttp)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
add_library(${PROJECT_NAME} ${SRC_LIST})

set (CMAKE_CXX_COMPILER  "/usr/bin/clang++")
set (CMAKE_CXX_FLAGS     "-Weverything -Wno-c++98-compat -Wno-padded")

# Enable C++14
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

# install target
install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES httprequest.h DESTINATION include)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
