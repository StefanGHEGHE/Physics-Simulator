cmake_minimum_required(VERSION 3.13)

project(PhysicEngine C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(exec src/main.c src/glad.c)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

target_include_directories(exec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_directories(exec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(exec glfw OpenGL::GL m)