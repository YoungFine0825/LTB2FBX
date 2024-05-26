cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

set(EXE_NAME ltb2fbx)

project(${EXE_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE HEADERS *.h)
file(GLOB_RECURSE SOURCES *.cpp)

add_executable(${EXE_NAME} ${HEADERS} ${SOURCES})
target_include_directories(${EXE_NAME} PRIVATE 
    ./
    ${THIRD_PARTY_INCLUDES_DIR}
)
target_link_directories(${EXE_NAME} PRIVATE ${THIRD_PARTY_LIBS_DIR})
target_link_libraries(${EXE_NAME} PRIVATE ${THIRD_PARTY_LIBS_NAME})

add_custom_command(
    TARGET ${EXE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${EXE_NAME}> "${PROJECT_ROOT_DIR}/Test/"
)
