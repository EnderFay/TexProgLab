cmake_minimum_required(VERSION 3.10)
project(RestaurantNetwork LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(server
    server.cpp
    Realization_of_clode_monet.cpp
)

add_executable(client
    clode_monet.cpp
)


if(WIN32)
    target_link_libraries(server PRIVATE ws2_32)
    target_link_libraries(client PRIVATE ws2_32)
endif()

configure_file(menu.txt ${CMAKE_BINARY_DIR}/menu.txt COPYONLY)
configure_file(users.txt ${CMAKE_BINARY_DIR}/users.txt COPYONLY)
configure_file(orders.txt ${CMAKE_BINARY_DIR}/orders.txt COPYONLY)

file(WRITE ${CMAKE_BINARY_DIR}/menu.txt "Бургер:150\nКола:50\n")
file(WRITE ${CMAKE_BINARY_DIR}/users.txt "")
file(WRITE ${CMAKE_BINARY_DIR}/orders.txt "")