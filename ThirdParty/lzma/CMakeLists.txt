cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

set(LIB_NAME lzma)

project(${LIB_NAME} VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LZMA_INCLUDE_DIR
    ./
)

file(GLOB LZMA_SOURCES "*.c")

add_library(${LIB_NAME} STATIC ${LZMA_SOURCES})
target_include_directories(${LIB_NAME} PRIVATE ${LZMA_INCLUDE_DIR})
set_property(TARGET ${LIB_NAME} PROPERTY FOLDER "ThirdParty")