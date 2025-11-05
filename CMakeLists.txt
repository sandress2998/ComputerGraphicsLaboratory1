cmake_minimum_required(VERSION 3.10)
project(PngProject)

set(CMAKE_CXX_STANDARD 17)

# Явно указываем где искать библиотеки (если нужно)
set(CMAKE_PREFIX_PATH "C:/msys64/mingw64")

find_package(PNG REQUIRED)

add_executable(my_program2 main.cpp)
target_link_libraries(my_program2 PNG::PNG)
