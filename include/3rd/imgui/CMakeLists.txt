project(imgui)
set(THIS_PROJECT_NAME "imgui")


file(GLOB SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/imgui)

add_library(${THIS_PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${THIS_PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui)


