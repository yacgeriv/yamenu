cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_C_FLAGS "-g")

file(GLOB SRC_FILES vendor/glad/glad/src/*.c)

if(EXISTS "build/compile_commands.json")
    file(COPY build/compile_commands.json DESTINATION ${CMAKE_SOURCE_DIR})
endif()

file(COPY vertex.glsl DESTINATION ${CMAKE_SOURCE_DIR}/build)
file(COPY fragment.glsl DESTINATION ${CMAKE_SOURCE_DIR}/build)
if(EXISTS "vendor/SDL/build/SDL3.dll") 
    file(COPY vendor/SDL/build/SDL3.dll DESTINATION ${CMAKE_SOURCE_DIR}/build)
endif()
project(yamenu)

add_executable(yamenu main.c src/yamenu.c ${SRC_FILES})
if(WIN32)
    target_link_libraries(yamenu PRIVATE libSDL3.dll.a libcglm.dll.a m libfreetype.dll.a)
else()
    find_package(Freetype REQUIRED)
    target_link_libraries(yamenu PRIVATE libSDL3.so libcglm.so m ${FREETYPE_LIBRARIES})
endif()
target_include_directories(yamenu PRIVATE vendor/cglm/include)
target_link_directories(yamenu PRIVATE vendor/cglm/build/)
if(WIN32) 
    target_include_directories(yamenu PRIVATE vendor/freetype/include)
    target_link_directories(yamenu PRIVATE vendor/freetype/lib/)
else()
    target_include_directories(yamenu PRIVATE ${FREETYPE_INCLUDE_DIRS})
endif()
target_include_directories(yamenu PRIVATE vendor/glad/glad/include)

target_include_directories(yamenu PRIVATE vendor/SDL/include)
target_link_directories(yamenu PRIVATE vendor/SDL/build/)

target_include_directories(yamenu PRIVATE include/)

target_include_directories(yamenu PRIVATE vendor/stb/)
