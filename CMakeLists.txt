cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(ParticleSim VERSION 0.0.1)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if(APPLE)
    set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang")
    set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-autolink")
else()
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

set(SDL_STATIC ON)
include(externalFunctions.cmake)

# -------------------------------------------
# macOS 特定配置
# -------------------------------------------
if(APPLE)
    
    # 解决标准库可用性问题
    add_compile_options(-D_LIBCPP_DISABLE_AVAILABILITY)
    
    # 设置 macOS 部署目标版本（根据需要调整）
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum macOS deployment version")

    # 修复数学函数问题
    add_compile_options(-D_DARWIN_C_SOURCE)
    
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
# 查找 Vulkan SDK
if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "VULKAN_SDK environment variable not set!")
else()
    set(VULKAN_SDK $ENV{VULKAN_SDK})
endif()

if(APPLE)
    # macOS 上使用 MoltenVK
    set(MOLTENVK_PATHS
        /Users/yinghai/VulkanSDK/1.4.313.1/macOS
        # /usr/local        # Homebrew 默认安装路径
        #/opt/homebrew     # Apple Silicon Homebrew 路径
        # ${CMAKE_BINARY_DIR}/_deps # 如果通过 CMake 下载
        # ${CMAKE_CURRENT_SOURCE_DIR}/third_party # 本地副本
    )
    
    # 查找 MoltenVK 头文件
    find_path(MoltenVK_INCLUDE_DIR
        NAMES MoltenVK/mvk_vulkan.h
        PATHS ${MOLTENVK_PATHS}
        NO_DEFAULT_PATH
        PATH_SUFFIXES include
        DOC "MoltenVK include directory"
    )
    
    # 查找 MoltenVK 库文件
    find_library(MoltenVK_LIBRARY
        NAMES MoltenVK
        PATHS ${MOLTENVK_PATHS}
        NO_DEFAULT_PATH
        PATH_SUFFIXES lib
        DOC "MoltenVK library"
    )
    
    # 查找 Vulkan 框架
    find_library(VULKAN_FRAMEWORK
        NAMES vulkan
        PATHS ${MOLTENVK_PATHS}
        NO_DEFAULT_PATH
        PATH_SUFFIXES lib
        DOC "Vulkan framework"
    )

    # 验证找到的路径
    if(NOT MoltenVK_INCLUDE_DIR)
        message(FATAL_ERROR "❌ MoltenVK include directory not found. Install with: brew install molten-vk")
    endif()
    
    if(NOT MoltenVK_LIBRARY)
        message(FATAL_ERROR "❌ MoltenVK library not found. Install with: brew install molten-vk")
    endif()
    
    if(NOT VULKAN_FRAMEWORK)
        message(FATAL_ERROR "❌ Vulkan framework not found. Install with: brew install vulkan-headers")
    endif()
    
    message(STATUS "✅ Found MoltenVK include at: ${MoltenVK_INCLUDE_DIR}")
    message(STATUS "✅ Found MoltenVK library at: ${MoltenVK_LIBRARY}")
    message(STATUS "✅ Found Vulkan framework at: ${VULKAN_FRAMEWORK}")

    # include_directories(/Users/yinghai/develepment/VulkanSDK/1.3.296.0/macOS/include/)
    # link_directories(/Users/yinghai/develepment/VulkanSDK/1.3.296.0/macOS/lib)
    # set(LIB_VULKAN "/Users/yinghai/develepment/VulkanSDK/1.3.296.0/macOS/lib/libvulkan.dylib")
    find_package(Vulkan REQUIRED)

    # 创建 Vulkan 目标
    # add_library(Vulkan::Vulkan INTERFACE IMPORTED)
    # target_include_directories(Vulkan::Vulkan INTERFACE
    #     ${MoltenVK_INCLUDE_DIR}
    #     ${MoltenVK_INCLUDE_DIR}/MoltenVK
    # )
    # target_link_libraries(Vulkan::Vulkan INTERFACE
    #     ${MoltenVK_LIBRARY}
    #     # ${VULKAN_FRAMEWORK}
    #     # ${METAL_LIBRARY}
    #     # ${QUARTZCORE_LIBRARY}
    #     # ${COREFOUNDATION_LIBRARY}
    # )
    
    
    # # 添加必要的编译定义
    # target_compile_definitions(Vulkan::Vulkan INTERFACE
    #     VK_USE_PLATFORM_MACOS_MVK
    #     VK_NO_PROTOTYPES
    # )

    # 添加 macOS 特定的链接选项
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Metal -framework MetalKit -framework QuartzCore")
else()
    # 其他平台使用标准 Vulkan
    if(NOT DEFINED VULKAN_SDK_PATH)
        
        if(DEFINED ENV{VULKAN_SDK})
            set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
            message(STATUS "Using VULKAN_SDK environment variable: ${VULKAN_SDK_PATH}")
        endif()
    endif()
    find_package(Vulkan REQUIRED)
endif()
    message(STATUS "Vulkan IncludeDIR found: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "Vulkan Library found: ${Vulkan_LIBRARY}")
    message(STATUS "Vulkan found: ${Vulkan_FOUND}")

# ----------------------------------------
# 创建可执行文件
# ----------------------------------------
file(GLOB_RECURSE PROJECT_RENDER_SOURCE_FILES "src/render/*.cpp")
file(GLOB_RECURSE PROJECT_RENDER_HEADER_FILES "src/render/*.h")
file(GLOB_RECURSE PROJECT_HEADER_DIRS "src/*.h")

add_executable(${PROJECT_NAME} 
src/main.cpp
src/ParticleSim.cpp
src/Hud.cpp
${PROJECT_RENDER_SOURCE_FILES}
)

# Debug still has symbols, but the cell loop is too slow at -O0 to hold 60 fps.
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>>:-O2>
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/O2>
)

find_program(GLSLANG_VALIDATOR glslangValidator
    HINTS "$ENV{VULKAN_SDK}/macOS/bin" "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin"
    REQUIRED
)
set(PARTICLE_VERT_SPV "${CMAKE_BINARY_DIR}/shaders/main.vert.spv")
set(PARTICLE_FRAG_SPV "${CMAKE_BINARY_DIR}/shaders/main.frag.spv")
file(TO_CMAKE_PATH "${PARTICLE_VERT_SPV}" PARTICLE_VERT_SPV_POSIX)
file(TO_CMAKE_PATH "${PARTICLE_FRAG_SPV}" PARTICLE_FRAG_SPV_POSIX)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
file(WRITE "${CMAKE_BINARY_DIR}/generated/shader_path.h"
"#pragma once\n#define PARTICLESIM_VERT_SPV \"${PARTICLE_VERT_SPV_POSIX}\"\n#define PARTICLESIM_FRAG_SPV \"${PARTICLE_FRAG_SPV_POSIX}\"\n"
)
add_custom_command(
    OUTPUT "${PARTICLE_VERT_SPV}" "${PARTICLE_FRAG_SPV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
    COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.2
            "${CMAKE_SOURCE_DIR}/assets/shaders/main.vert" -o "${PARTICLE_VERT_SPV}"
    COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.2
            "${CMAKE_SOURCE_DIR}/assets/shaders/main.frag" -o "${PARTICLE_FRAG_SPV}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/assets/shaders/main.vert"
        "${CMAKE_SOURCE_DIR}/assets/shaders/main.frag"
    VERBATIM
)
add_custom_target(ParticleSimShaders DEPENDS "${PARTICLE_VERT_SPV}" "${PARTICLE_FRAG_SPV}")
add_dependencies(${PROJECT_NAME} ParticleSimShaders)

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
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/generated
    third_party/stb
)

if(APPLE)

    target_link_directories(ParticleSim PRIVATE
        ${VULKAN_SDK}/macOS/lib
    )
    target_link_directories(ParticleSim PRIVATE
        ${VULKAN_SDK}/macOS/Frameworks
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
            # ${Vulkan_LIBRARY}
            # "-framework vulkan"
            Vulkan::Vulkan
            "-framework Metal"
            "-framework MetalKit"
            "-framework QuartzCore"
            "-framework Cocoa"
        )
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            ${Vulkan_LIBRARY}
        )
endif()

# 链接库文件
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3-static        # SDL3 库
    glm                      # glm 库
    spdlog                   # spdlog 库
)

# macOS 需要链接的系统框架
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
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
    PROJECT_VK_VERSION_MINOR=4
    PROJECT_VK_VERSION_PATCH=313

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
