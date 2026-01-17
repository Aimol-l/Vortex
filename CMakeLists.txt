cmake_minimum_required(VERSION 3.25)
project(vortex VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
# 默认构建类型为 Debug，可通过命令行修改: cmake -DCMAKE_BUILD_TYPE=Release ..
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出构建类型信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}") 

get_filename_component(PROJECT_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_DIR}/bin)


include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/3rd
    ${CMAKE_SOURCE_DIR}/include/Assets
    ${CMAKE_SOURCE_DIR}/include/Core
    ${CMAKE_SOURCE_DIR}/include/Scene
)

# 明确列出源文件，避免重复包含 VMA
set(SRC_FILES
    # 主程序
    "${PROJECT_SOURCE_DIR}/test/vortex.cpp"

    # Application 层
    "${PROJECT_SOURCE_DIR}/src/Application.cpp"

    # Core 层
    "${PROJECT_SOURCE_DIR}/src/Core/VMAImpl.cpp"  # VMA 实现，必须先包含
    "${PROJECT_SOURCE_DIR}/src/Core/Context.cpp"
    "${PROJECT_SOURCE_DIR}/src/Core/Renderer.cpp"
    "${PROJECT_SOURCE_DIR}/src/Core/Swapchain.cpp"
    "${PROJECT_SOURCE_DIR}/src/Core/Command.cpp"
    "${PROJECT_SOURCE_DIR}/src/Core/Pipeline.cpp"
    "${PROJECT_SOURCE_DIR}/src/Core/RenderPass.cpp"
    "${PROJECT_SOURCE_DIR}/src/Core/Descriptor.cpp"
    "${PROJECT_SOURCE_DIR}/src/Core/Window.cpp"
    "${PROJECT_SOURCE_DIR}/src/Core/Inputs.cpp"

    # Scene 层
    "${PROJECT_SOURCE_DIR}/src/Scene/Scene.cpp"

    # Assets 层
    "${PROJECT_SOURCE_DIR}/src/Assets/Mesh.cpp"
    "${PROJECT_SOURCE_DIR}/src/Assets/Material.cpp"
    "${PROJECT_SOURCE_DIR}/src/Assets/Texture.cpp"
)
include_directories(${PROJECT_DIR}/include)


# Find packages
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_FILES})

# 添加调试符号
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -g -O0)
    message(STATUS "Debug build with symbols enabled")
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${glm_INCLUDE_DIRS})

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC glfw)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC glm)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES})