cmake_minimum_required(VERSION 3.15)
project(PongGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_executable(PongGame main.cpp)

target_include_directories(PongGame PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    /usr/include/SDL2
)

target_link_libraries(PongGame PRIVATE
    ${SDL2_LIBRARIES}
    SDL2_ttf
    SDL2_gfx
)
