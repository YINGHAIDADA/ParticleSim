# 函数：移动子目录内容到父目录并删除原子目录
# 参数:
#   PARENT_DIR - 包含唯一子目录的父目录路径
function(move_and_remove_with_system_cmd PARENT_DIR)
    # 验证父目录存在
    if(NOT IS_DIRECTORY "${PARENT_DIR}")
        message(FATAL_ERROR "Parent directory does not exist: ${PARENT_DIR}")
    endif()
    
    # 获取父目录下的所有子目录
    file(GLOB SUBDIRS LIST_DIRECTORIES true RELATIVE "${PARENT_DIR}" "${PARENT_DIR}/*")
    
    # 过滤出实际目录
    set(DIR_LIST)
    foreach(ITEM IN LISTS SUBDIRS)
        if(IS_DIRECTORY "${PARENT_DIR}/${ITEM}")
            list(APPEND DIR_LIST "${ITEM}")
        endif()
    endforeach()
    
    # 检查目录数量
    list(LENGTH DIR_LIST DIR_COUNT)
    if(DIR_COUNT EQUAL 0)
        message(FATAL_ERROR "No subdirectory found in: ${PARENT_DIR}")
    elseif(DIR_COUNT GREATER 1)
        message(FATAL_ERROR "Expected exactly one directory, found ${DIR_COUNT}: ${DIR_LIST}")
    endif()
    
    # 获取唯一子目录名称
    list(GET DIR_LIST 0 SUBDIR_NAME)
    set(SUBDIR_PATH "${PARENT_DIR}/${SUBDIR_NAME}")
    
    # 跨平台移动命令
    if(UNIX)
        # Linux/macOS: 移动所有内容（包括隐藏文件）
        execute_process(COMMAND bash -c "shopt -s dotglob && mv ${SUBDIR_PATH}/* ${PARENT_DIR}/"
            WORKING_DIRECTORY ${PARENT_DIR}
            RESULT_VARIABLE RESULT
        )
    elseif(WIN32)
        # Windows: 使用robocopy移动
        execute_process(COMMAND robocopy "${SUBDIR_PATH}" "${PARENT_DIR}" /MOVE /E /NFL /NDL /NJH /NJS
            WORKING_DIRECTORY ${PARENT_DIR}
            RESULT_VARIABLE RESULT
        )
        # robocopy返回0-7表示成功
        if(RESULT EQUAL 0 OR RESULT EQUAL 1 OR RESULT EQUAL 2 OR RESULT EQUAL 3)
            set(RESULT 0)
        endif()
    endif()
    
    # 检查命令执行结果
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to move contents: error ${RESULT}")
    endif()
    
    # 删除空目录
    file(REMOVE_RECURSE "${SUBDIR_PATH}")
endfunction()



# ================================================================
# 功能：下载并解压 GitHub release
# 参数：
#   REPO: 仓库路径 (格式: "user/repo")
#   TAG:  release 标签 (如 "v1.0.0")
#   DOWNLOAD_DIR: 下载存放目录
#   EXTRACT_TO: 解压目标目录
# ================================================================
function(download_github_release)
    set(options)
    set(oneValueArgs OWNER REPO TAG ARCHIVE_NAME DOWNLOAD_DIR EXTRACT_TO)
    cmake_parse_arguments(DGR "${options}" "${oneValueArgs}" "" ${ARGN})
    
    # 验证参数\
    if(NOT DGR_OWNER)
        message(FATAL_ERROR "OWNER argument is required")
    endif()
    if(NOT DGR_REPO)
        message(FATAL_ERROR "REPO argument is required")
    endif()
    if(NOT DGR_TAG)
        message(FATAL_ERROR "TAG argument is required")
    endif()
    if(NOT DGR_ARCHIVE_NAME)
        message(FATAL_ERROR "ARCHIVE_NAME argument is required")
    endif()
    if(NOT DGR_DOWNLOAD_DIR)
        set(DGR_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/downloads")
    endif()
    if(NOT DGR_EXTRACT_TO)
        set(DGR_EXTRACT_TO "${CMAKE_BINARY_DIR}/extracted")
    endif()
    
    # 创建下载目录
    file(MAKE_DIRECTORY ${DGR_DOWNLOAD_DIR})
    
    # 构造下载 URL (优先使用 tar.gz)
    set(DOWNLOAD_URL "https://github.com/${DGR_OWNER}/${DGR_REPO}/releases/download/${DGR_TAG}/${DGR_ARCHIVE_NAME}")
    set(DOWNLOAD_PATH "${DGR_DOWNLOAD_DIR}/${DGR_ARCHIVE_NAME}")
    
    message(STATUS "Downloading ${DOWNLOAD_URL}")
    
    # 下载文件（如果不存在）
    if(NOT EXISTS ${DOWNLOAD_PATH})
        file(DOWNLOAD 
            ${DOWNLOAD_URL} 
            ${DOWNLOAD_PATH}
            SHOW_PROGRESS
            STATUS download_status
        )
        
        list(GET download_status 0 status_code)
        if(NOT status_code EQUAL 0)
            list(GET download_status 1 error_msg)
            message(FATAL_ERROR "Download failed: ${error_msg}")
        endif()
    else()
        message(STATUS "File already exists, skipping download: ${DOWNLOAD_PATH}")
    endif()
    
    # 创建解压目录
    file(MAKE_DIRECTORY ${DGR_EXTRACT_TO})
    
    # 解压文件
    message(STATUS "Extracting to: ${DGR_EXTRACT_TO}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${DOWNLOAD_PATH}
        WORKING_DIRECTORY ${DGR_EXTRACT_TO}
        RESULT_VARIABLE extract_result
    )
    
    if(NOT extract_result EQUAL 0)
        message(FATAL_ERROR "Extraction failed with code: ${extract_result}")
    endif()
    
    # 重命名解压后的文件夹
    move_and_remove_with_system_cmd("${DGR_EXTRACT_TO}")
    
    message(STATUS "Download and extraction complete: ${TARGET_FOLDER}")
endfunction()

function(download_all_dependencies)
    # ================================================================
    # 下载 SDL
    # ================================================================
    if(NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/SDL3")
        download_github_release(
            OWNER "libsdl-org"
            REPO "SDL"
            TAG "release-3.2.14"
            ARCHIVE_NAME "SDL3-devel-3.2.14-VC.zip"
            DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/downloads"
            EXTRACT_TO "${CMAKE_SOURCE_DIR}/third_party/SDL3"
        )
    else()
        message(STATUS "SDL3 already Exist.")
    endif()

    # ================================================================
    # 下载 SDL_image
    # ================================================================
    if(NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/SDL3_image")
        download_github_release(
            OWNER "libsdl-org"
            REPO "SDL_image"
            TAG "release-3.2.4"
            ARCHIVE_NAME "SDL3_image-devel-3.2.4-VC.zip"
            DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/downloads"
            EXTRACT_TO "${CMAKE_SOURCE_DIR}/third_party/SDL3_image"
        )
    else()
        message(STATUS "SDL3_image already Exist.")
    endif()

    # ================================================================
    # 下载 SDL_mixer
    # ================================================================
    if(NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/SDL2_mixer")
        download_github_release(
            OWNER "libsdl-org"
            REPO "SDL_mixer"
            TAG "release-2.8.1"
            ARCHIVE_NAME "SDL2_mixer-devel-2.8.1-VC.zip"
            DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/downloads"
            EXTRACT_TO "${CMAKE_SOURCE_DIR}/third_party/SDL2_mixer"
        )
    else()
        message(STATUS "SDL2_mixer already Exist.")
    endif()

    # ================================================================
    # 下载 SDL_ttf
    # ================================================================
    if(NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/SDL3_ttf")
        download_github_release(
            OWNER "libsdl-org"
            REPO "SDL_ttf"
            TAG "release-3.2.2"
            ARCHIVE_NAME "SDL3_ttf-devel-3.2.2-VC.zip"
            DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/downloads"
            EXTRACT_TO "${CMAKE_SOURCE_DIR}/third_party/SDL3_ttf"
        )
    else()
        message(STATUS "SDL3_ttf already Exist.")
    endif()
endfunction()