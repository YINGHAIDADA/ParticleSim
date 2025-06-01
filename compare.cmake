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
# macOS 特定配置
# -------------------------------------------
if(APPLE)
    # 设置 macOS 部署目标版本（根据需要调整）
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
    
    # 启用 ARC (Automatic Reference Counting)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
    
    # 启用 Objective-C 异常处理
    set(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_CPP_EXCEPTIONS "YES")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_OBJC_EXCEPTIONS "YES")
    
    # 禁用 rpath 警告
    set(CMAKE_MACOSX_RPATH ON)
    
    # 设置 macOS 框架搜索路径
    list(APPEND CMAKE_FIND_FRAMEWORK "LAST")
    
    # 查找 macOS 系统框架
    find_library(COCOA_LIBRARY Cocoa)
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREGRAPHICS_LIBRARY CoreGraphics)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
    find_library(AUDIOUNIT_LIBRARY AudioUnit)
    find_library(CARBON_LIBRARY Carbon)
    find_library(FORCEFEEDBACK_LIBRARY ForceFeedback)
endif()

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

# ----------------------------------------
# 配置 Vulkan (macOS 使用 MoltenVK)
# ----------------------------------------
if(APPLE)
    # macOS 上使用 MoltenVK
    find_package(MoltenVK REQUIRED)
    
    message(STATUS "MoltenVK found: ${MoltenVK_INCLUDE_DIR}")
    message(STATUS "MoltenVK libraries: ${MoltenVK_LIBRARIES}")
    
    # 创建 Vulkan 目标
    add_library(Vulkan::Vulkan INTERFACE IMPORTED)
    target_include_directories(Vulkan::Vulkan INTERFACE
        ${MoltenVK_INCLUDE_DIR}
    )
    target_link_libraries(Vulkan::Vulkan INTERFACE
        ${MoltenVK_LIBRARIES}
    )
    target_link_libraries(Vulkan::Vulkan INTERFACE
        ${METAL_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
    )
    
    # 添加必要的编译定义
    target_compile_definitions(Vulkan::Vulkan INTERFACE
        VK_USE_PLATFORM_MACOS_MVK
        VK_NO_PROTOTYPES
    )
else()
    # 其他平台使用标准 Vulkan
    find_package(Vulkan REQUIRED)
    message(STATUS "Vulkan found: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "Vulkan found: ${Vulkan_LIBRARY}")
    message(STATUS "Vulkan found: ${Vulkan_FOUND}")
endif()

# ----------------------------------------
# 创建可执行文件
# ----------------------------------------
file(GLOB_RECURSE PROJECT_RENDER_SOURCE_FILES "src/render/*.cpp")
file(GLOB_RECURSE PROJECT_HEADER_DIRS "src/*.h")

add_executable(${PROJECT_NAME} 
src/main.cpp
src/ParticleSim.cpp
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

# macOS 需要链接的系统框架
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_LIBRARY}
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${AUDIOUNIT_LIBRARY}
        ${CARBON_LIBRARY}
        ${FORCEFEEDBACK_LIBRARY}
    )
endif()

# Windows 特定配置
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
    # 链接 Windows 系统库
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32.lib
        shell32.lib
        dxguid.lib
    )
endif()

# ----------------------------------------
# macOS 应用包配置（可选）
# ----------------------------------------
if(APPLE)
    # 设置应用包属性
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2023 Your Company")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.yinghaidada.${PROJECT_NAME}")
    set(MACOSX_BUNDLE_ICON_FILE "")
    set(MACOSX_BUNDLE_INFO_STRING "")
    
    # 配置为应用包
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macOS/Info.plist.in"
    )
    
    # 添加资源目录
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/resources"
                "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources"
        )
    endif()
    
    # 复制 Vulkan 相关库到应用包
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${MoltenVK_LIBRARY}"
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks"
    )
endif()