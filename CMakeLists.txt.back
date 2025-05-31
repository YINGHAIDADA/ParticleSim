cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(ParticleSim VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

set(SDL_STATIC ON)
include(externalFunctions.cmake)

# 下载所有依赖
download_all_dependencies()
# ----------------------------------------
# 配置 SDL3 路径（项目内第三方库）
# ----------------------------------------
set(SDL3_BASE_DIR "${CMAKE_SOURCE_DIR}/third_party/SDL3")
set(SDL3_IMAGE_BASE_DIR "${CMAKE_SOURCE_DIR}/third_party/SDL3_image")
set(SDL2_MIXER_BASE_DIR "${CMAKE_SOURCE_DIR}/third_party/SDL2_mixer")
set(SDL3_TTF_BASE_DIR "${CMAKE_SOURCE_DIR}/third_party/SDL3_ttf")

# 头文件路径（直接指定）
set(SDL3_INCLUDE_DIR "${SDL3_BASE_DIR}/include")
set(SDL3_IMAGE_INCLUDE_DIR "${SDL3_IMAGE_BASE_DIR}/include")
set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_BASE_DIR}/include")
set(SDL3_TTF_INCLUDE_DIR "${SDL3_TTF_BASE_DIR}/include")

# 动态库/静态库路径（自动适配平台和架构）
if(WIN32)
    # 自动检测架构（x64 或 x86）
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL3_LIB_ARCH_DIR "x64")
    else()
        set(SDL3_LIB_ARCH_DIR "x86")
    endif()

    # 设置库文件搜索路径
    set(SDL3_LIB_DIR "${SDL3_BASE_DIR}/lib/${SDL3_LIB_ARCH_DIR}")
    set(SDL3_IMAGE_LIB_DIR "${SDL3_IMAGE_BASE_DIR}/lib/${SDL3_LIB_ARCH_DIR}")
    set(SDL2_MIXER_LIB_DIR "${SDL2_MIXER_BASE_DIR}/lib/${SDL3_LIB_ARCH_DIR}")
    set(SDL3_TTF_LIB_DIR "${SDL3_TTF_BASE_DIR}/lib/${SDL3_LIB_ARCH_DIR}")
else()
    # Linux/macOS 使用统一路径
    message(STATUS "SDL3_LIB_DIR: ${SDL3_LIB_DIR}")
    set(SDL3_LIB_DIR "${SDL3_BASE_DIR}/lib/arm64")
    set(SDL3_IMAGE_LIB_DIR "${SDL3_IMAGE_BASE_DIR}/lib/arm64")
    set(SDL2_MIXER_LIB_DIR "${SDL2_MIXER_BASE_DIR}/lib/${SDL3_LIB_ARCH_DIR}")
    set(SDL3_TTF_LIB_DIR "${SDL3_TTF_BASE_DIR}/lib/arm64")
endif()

# 查找 SDL3 库文件
find_library(SDL3_LIBRARY
    NAMES SDL3 SDL3-static   # 动态库和静态库名称
    PATHS ${SDL3_LIB_DIR}
    NO_DEFAULT_PATH          # 禁止搜索系统路径
)
message(STATUS "SDL3_LIBRARY: ${SDL3_LIBRARY}")
message(STATUS "SDL3_INCLUDE_DIR: ${SDL3_INCLUDE_DIR}")

if(NOT SDL3_LIBRARY)
    message(FATAL_ERROR "SDL3 library not found in: ${SDL3_LIB_DIR}")
endif()

# 查找 SDL3_image 库文件
find_library(SDL3_IMAGE_LIBRARY
    NAMES SDL3_image SDL3_image-static   # 动态库和静态库名称
    PATHS ${SDL3_IMAGE_LIB_DIR}
    NO_DEFAULT_PATH          # 禁止搜索系统路径
)
message(STATUS "SDL3_IMAGE_LIBRARY: ${SDL3_IMAGE_LIBRARY}")
message(STATUS "SDL3_IMAGE_INCLUDE_DIR: ${SDL3_IMAGE_INCLUDE_DIR}")

if(NOT SDL3_IMAGE_LIBRARY)
    message(FATAL_ERROR "SDL3_image library not found in: ${SDL3_IMAGE_LIB_DIR}")
endif()

# 查找 SDL2_mixer 库文件
find_library(SDL2_MIXER_LIBRARY
    NAMES SDL2_mixer SDL2_mixer-static   # 动态库和静态库名称
    PATHS ${SDL2_MIXER_LIB_DIR}
    NO_DEFAULT_PATH          # 禁止搜索系统路径
)
message(STATUS "SDL2_MIXER_LIBRARY: ${SDL2_MIXER_LIBRARY}")
message(STATUS "SDL2_MIXER_INCLUDE_DIR: ${SDL2_MIXER_INCLUDE_DIR}")

if(NOT SDL2_MIXER_LIBRARY)
    message(FATAL_ERROR "SDL2_mixer library not found in: ${SDL2_MIXER_LIB_DIR}")
endif()

# 查找 SDL3_ttf 库文件
find_library(SDL3_TTF_LIBRARY
    NAMES SDL3_ttf SDL3_ttf-static   # 动态库和静态库名称
    PATHS ${SDL3_TTF_LIB_DIR}
    NO_DEFAULT_PATH          # 禁止搜索系统路径
)
message(STATUS "SDL3_TTF_LIBRARY: ${SDL3_TTF_LIBRARY}")
message(STATUS "SDL3_TTF_INCLUDE_DIR: ${SDL3_TTF_INCLUDE_DIR}")
if(NOT SDL3_TTF_LIBRARY)
    message(FATAL_ERROR "SDL3_ttf library not found in: ${SDL3_TTF_LIB_DIR}")
endif()

# ----------------------------------------
# 配置 Vulkan
# ----------------------------------------
find_package(Vulkan REQUIRED)
message(STATUS "Vulkan found: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan found: ${Vulkan_LIBRARY}")
message(STATUS "Vulkan found: ${Vulkan_FOUND}")


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

message(NOTICE "🔷 load glm")
FetchContent_MakeAvailable(glm)
message(NOTICE "✅ glm loaded")

message(NOTICE "🔷 load spdlog")
FetchContent_MakeAvailable(spdlog)
message(NOTICE "✅ spdlog loaded")

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
    ${SDL3_INCLUDE_DIR}       # SDL3 头文件路径
    ${SDL3_IMAGE_INCLUDE_DIR} # SDL3_image 头文件路径
    ${SDL2_MIXER_INCLUDE_DIR} # SDL2_mixer 头文件路径
    ${SDL3_TTF_INCLUDE_DIR}   # SDL3_ttf 头文件路径
    ${Vulkan_INCLUDE_DIR}     # Vulkan 头文件
    ${INCLUDE_DIRS}
    third_party/stb
)

# 链接库文件
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan           # Vulkan 库
    ${SDL3_LIBRARY}          # SDL3 库文件
    ${SDL3_IMAGE_LIBRARY}    # SDL3_image 库文件
    ${SDL2_MIXER_LIBRARY}    # SDL2_mixer 库文件
    ${SDL3_TTF_LIBRARY}      # SDL3_ttf 库文件
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
    if(EXISTS "${SDL3_LIB_DIR}/SDL3.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${SDL3_LIB_DIR}/SDL3.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    else()
        message(WARNING "SDL3.dll not found in ${SDL3_LIB_DIR}")
    endif()

    # 自动复制 SDL3_image.dll 到输出目录
    if(EXISTS "${SDL3_IMAGE_LIB_DIR}/SDL3_image.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${SDL3_IMAGE_LIB_DIR}/SDL3_image.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    else()
        message(WARNING "SDL3_image.dll not found in ${SDL3_IMAGE_LIB_DIR}")
    endif()

    # 自动复制 SDL2_mixer.dll 到输出目录
    if(EXISTS "${SDL2_MIXER_LIB_DIR}/SDL2_mixer.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${SDL2_MIXER_LIB_DIR}/SDL2_mixer.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    else()
        message(WARNING "SDL2_mixer.dll not found in ${SDL2_MIXER_LIB_DIR}")
    endif()

    # 自动复制 SDL3_ttf.dll 到输出目录
    if(EXISTS "${SDL3_TTF_LIB_DIR}/SDL3_ttf.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${SDL3_TTF_LIB_DIR}/SDL3_ttf.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    else()
        message(WARNING "SDL3_ttf.dll not found in ${SDL3_TTF_LIB_DIR}")
    endif()

    # 链接 Windows 系统库
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32.lib
        shell32.lib
        dxguid.lib
    )
endif()
