set(INCLUDES_DIR "")
set(LIBS_DIR "")
set(LIBS_NAME "")

#assimp
if(ASSIMP_STATIC_LIB)
set(BUILD_SHARED_LIBS OFF)
else()
set(BUILD_SHARED_LIBS ON)
endif()
add_subdirectory(assimp)
list(APPEND INCLUDES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include)
list(APPEND LIBS_NAME assimp)
set_property(TARGET assimp        PROPERTY FOLDER "ThirdParty")

# lithtech engine library
add_subdirectory(lithtech)
list(APPEND INCLUDES_DIR 
    ${CMAKE_CURRENT_SOURCE_DIR}/lithtech
    ${CMAKE_CURRENT_SOURCE_DIR}/lithtech/runtime/kernel
    ${CMAKE_CURRENT_SOURCE_DIR}/lithtech/runtime/lithtemplate
    ${CMAKE_CURRENT_SOURCE_DIR}/lithtech/runtime/model
    ${CMAKE_CURRENT_SOURCE_DIR}/lithtech/runtime/render
    ${CMAKE_CURRENT_SOURCE_DIR}/lithtech/runtime/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/lithtech/sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/lithtech/sdk/physics
    ${CMAKE_CURRENT_SOURCE_DIR}/lithtech/sdk/sys
    ${CMAKE_CURRENT_SOURCE_DIR}/lithtech/stdlith
)
list(APPEND LIBS_NAME lithtech)

#lzma
add_subdirectory(lzma)
list(APPEND INCLUDES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lzma)
list(APPEND LIBS_NAME lzma)

set(THIRD_PARTY_INCLUDES_DIR "${THIRD_PARTY_INCLUDES_DIR};${INCLUDES_DIR}" PARENT_SCOPE)
set(THIRD_PARTY_LIBS_DIR "${THIRD_PARTY_LIBS_DIR};${LIBS_DIR}" PARENT_SCOPE)
set(THIRD_PARTY_LIBS_NAME "${THIRD_PARTY_LIBS_NAME};${LIBS_NAME}" PARENT_SCOPE)