cmake_minimum_required(VERSION 3.10)
project(RestaurantProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Сервер ===
add_executable(server
    server.cpp
    Realization_of_CLode.cpp

)

# === Клиент ===
add_executable(clode_monet
    CLode_monet_new.cpp
    Realization_of_CLode.cpp
)

# Тест кросс-платформенности
add_executable(test_platform test_platform_lab.cpp)
if(WIN32)
    target_link_libraries(test_platform ws2_32)
endif()

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
