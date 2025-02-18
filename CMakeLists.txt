cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_C_FLAGS "-g")

file(GLOB SRC_FILES vendor/glad/glad/src/*.c)
file(COPY build/compile_commands.json DESTINATION ${CMAKE_SOURCE_DIR})
file(COPY vertex.glsl DESTINATION ${CMAKE_SOURCE_DIR}/build)
file(COPY fragment.glsl DESTINATION ${CMAKE_SOURCE_DIR}/build)
file(COPY vendor/SDL/build/SDL3.dll DESTINATION ${CMAKE_SOURCE_DIR}/build)

project(yamenu)


add_executable(yamenu main.c ${SRC_FILES})

target_link_libraries(yamenu PRIVATE libSDL3.dll.a libcglm.dll.a m libfreetype.dll.a)

target_include_directories(yamenu PRIVATE vendor/cglm/include)
target_link_directories(yamenu PRIVATE vendor/cglm/build/)

target_include_directories(yamenu PRIVATE vendor/freetype/include)
target_link_directories(yamenu PRIVATE vendor/freetype/lib/)

target_include_directories(yamenu PRIVATE vendor/glad/glad/include)

target_include_directories(yamenu PRIVATE vendor/SDL/include)
target_link_directories(yamenu PRIVATE vendor/SDL/build/)

