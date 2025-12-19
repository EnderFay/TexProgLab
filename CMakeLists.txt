cmake_minimum_required(VERSION 3.10)
project(RestaurantProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === looking for spdlog ===
find_package(spdlog CONFIG REQUIRED)

# === Server ===
add_executable(server
    server.cpp
    Realization_of_CLode.cpp
    logger.cpp
)

# === Client ===
add_executable(clode_monet
    CLode_monet_new.cpp
    Realization_of_CLode.cpp
    logger.cpp
)

# === connection to spdlog ===
target_link_libraries(server PRIVATE spdlog::spdlog)
target_link_libraries(clode_monet PRIVATE spdlog::spdlog)

# === Winsock ===
if(WIN32)
    target_link_libraries(server PRIVATE Ws2_32)
    target_link_libraries(clode_monet PRIVATE Ws2_32)
endif()

# === copying files ===
file(GLOB DATA_FILES "*.txt")
foreach(file ${DATA_FILES})
    configure_file(${file} ${CMAKE_BINARY_DIR}/ COPYONLY)
endforeach()
