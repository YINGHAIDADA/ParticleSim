cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(ParticleSim VERSION 0.0.1)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

set(SDL_STATIC ON)
include(externalFunctions.cmake)

# -------------------------------------------
# 下载所有依赖
# -------------------------------------------
# 设置 spdlog 构建为静态库
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build spdlog as static library" FORCE)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)

FetchContent_Declare(
    sdl
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.14
)

message(NOTICE "🔷 load glm")
FetchContent_MakeAvailable(glm)
message(NOTICE "✅ glm loaded")

message(NOTICE "🔷 load spdlog")
FetchContent_MakeAvailable(spdlog)
message(NOTICE "✅ spdlog loaded")

message(NOTICE "🔷 load SDL3")
FetchContent_MakeAvailable(sdl)
message(NOTICE "✅ SDL3 loaded")

# 查找 SDL3 库文件
# find_library(SDL3_LIBRARY
#     NAMES SDL3 SDL3-static   # 动态库和静态库名称
# )
# message(STATUS "SDL3_LIBRARY: ${SDL3_LIBRARY}")

# if(NOT SDL3_LIBRARY)
#     message(FATAL_ERROR "SDL3 library not found in: ${SDL3_LIB_DIR}")
# endif()

# ----------------------------------------
# 配置 Vulkan
# ----------------------------------------
find_package(Vulkan REQUIRED)
message(STATUS "Vulkan found: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan found: ${Vulkan_LIBRARY}")
message(STATUS "Vulkan found: ${Vulkan_FOUND}")

# ----------------------------------------
# 创建可执行文件
# ----------------------------------------
file(GLOB_RECURSE PROJECT_RENDER_SOURCE_FILES "src/render/*.cpp")
file(GLOB_RECURSE PROJECT_HEADER_DIRS "src/*.h")

add_executable(${PROJECT_NAME} 
src/main.cpp
src/ParticleSim.cpp
#src/texture.c
# src/vulkan_sdl_demo.cpp
${PROJECT_RENDER_SOURCE_FILES}
)

set(INCLUDE_DIRS "")
foreach(HEADER ${PROJECT_HEADER_DIRS})
    get_filename_component(DIR ${HEADER} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${DIR})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

message(STATUS "🔷 include directories: ${INCLUDE_DIRS}")

# 添加头文件包含路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIR}     # Vulkan 头文件
    ${INCLUDE_DIRS}
    third_party/stb
)

# 链接库文件
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan           # Vulkan 库
    SDL3::SDL3-static        # SDL3 库
    glm                      # glm 库
    spdlog                   # spdlog 库
)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_NAME="${PROJECT_NAME}"
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PROJECT_VK_VERSION_MAJOR=1
    PROJECT_VK_VERSION_MINOR=3
    PROJECT_VK_VERSION_PATCH=268

    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<NOT:$<CONFIG:Debug>>:RELEASE_BUILD>
    $<$<BOOL:${WIN32}>:OS_WINDOWS>
    $<$<BOOL:${APPLE}>:OS_MACOS>
    $<$<BOOL:${UNIX}>:OS_LINUX>
)

# ----------------------------------------
# 平台特定配置（Windows）
# ----------------------------------------
if(WIN32)
    # 自动复制 SDL3.dll 到输出目录
    # if(EXISTS "${SDL3_LIB_DIR}/SDL3.dll")
    #     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    #         COMMAND ${CMAKE_COMMAND} -E copy
    #             "${SDL3_LIB_DIR}/SDL3.dll"
    #             "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    #     )
    # else()
    #     message(WARNING "SDL3.dll not found in ${SDL3_LIB_DIR}")
    # endif()

    # 链接 Windows 系统库
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32.lib
        shell32.lib
        dxguid.lib
    )
endif()
