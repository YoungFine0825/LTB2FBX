cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(LTB2FBX)

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(THIRD_PARTY_ROOT_DIR ${PROJECT_ROOT_DIR}/ThirdParty)
set(THIRD_PARTY_INCLUDES_DIR "")
set(THIRD_PARTY_LIBS_DIR "")
set(THIRD_PARTY_LIBS_NAME "")
set(ASSIMP_STATIC_LIB ON)#是否将assimp库作为静态库连接到exe中
add_subdirectory(ThirdParty)

add_subdirectory(Source)