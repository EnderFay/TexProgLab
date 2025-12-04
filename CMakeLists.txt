cmake_minimum_required(VERSION 3.10)
project(RestaurantProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Сервер ===
add_executable(server
    server.cpp
    Realization_of_clode_monet.cpp
)

# === Клиент ===
add_executable(clode_monet
    clode_monet.cpp
)

# === Winsock ===
if(WIN32)
    target_link_libraries(server PRIVATE Ws2_32)
    target_link_libraries(clode_monet PRIVATE Ws2_32)
endif()

# === Копируем файлы ===
file(GLOB DATA_FILES "*.txt")
foreach(file ${DATA_FILES})
    configure_file(${file} ${CMAKE_BINARY_DIR}/ COPYONLY)
endforeach()