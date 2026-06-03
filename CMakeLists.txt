cmake_minimum_required(VERSION 3.16)
project(SolarSystemMiniEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================
#  第三方库路径配置
#  优先级：cmake -D 参数 > 系统环境变量 > 自动查找
#
#  推荐调用方式（VS Developer PowerShell）：
#    cmake -B build -DGLEW_ROOT="C:/OpenGL/glew-2.2.0" ^
#                   -DFREEGLUT_ROOT="C:/OpenGL/freeglut-3.4.0"
# ==============================================================

# ── 从环境变量自动读取（用户已设置 GLEW_ROOT 系统变量）────────
if (NOT GLEW_ROOT AND DEFINED ENV{GLEW_ROOT})
    set(GLEW_ROOT "$ENV{GLEW_ROOT}" CACHE PATH "GLEW 安装根目录" FORCE)
    message(STATUS "GLEW_ROOT from ENV: ${GLEW_ROOT}")
endif()

if (NOT FREEGLUT_ROOT AND DEFINED ENV{FREEGLUT_ROOT})
    set(FREEGLUT_ROOT "$ENV{FREEGLUT_ROOT}" CACHE PATH "FreeGLUT 安装根目录" FORCE)
    message(STATUS "FREEGLUT_ROOT from ENV: ${FREEGLUT_ROOT}")
endif()

# ── 也可以在此处硬编码路径（CI / 无环境变量时使用）────────────
# set(GLEW_ROOT    "C:/OpenGL/glew-2.2.0")
# set(FREEGLUT_ROOT "C:/OpenGL/freeglut-3.4.0")

# ── OpenGL ────────────────────────────────────────────────────
find_package(OpenGL REQUIRED)

# ── GLEW ──────────────────────────────────────────────────────
if (GLEW_ROOT)
    # 手动导入：适用于官方预编译包的标准目录布局
    #   glew-2.2.0/
    #     include/GL/glew.h
    #     lib/Release/x64/glew32.lib   (动态链接版)
    #     lib/Release/x64/glew32s.lib  (静态链接版)
    #     bin/Release/x64/glew32.dll

    find_library(GLEW_LIB_PATH
        NAMES glew32 glew32s GLEW
        PATHS
            "${GLEW_ROOT}/lib/Release/x64"
            "${GLEW_ROOT}/lib/Release/Win32"
            "${GLEW_ROOT}/lib/x64"
            "${GLEW_ROOT}/lib"
        NO_DEFAULT_PATH
    )
    if (NOT GLEW_LIB_PATH)
        message(FATAL_ERROR
            "在 GLEW_ROOT='${GLEW_ROOT}' 下找不到 glew32.lib / glew32s.lib。\n"
            "请确认目录结构，例如：\n"
            "  ${GLEW_ROOT}/lib/Release/x64/glew32.lib")
    endif()

    add_library(GLEW::GLEW UNKNOWN IMPORTED)
    set_target_properties(GLEW::GLEW PROPERTIES
        IMPORTED_LOCATION             "${GLEW_LIB_PATH}"
        INTERFACE_INCLUDE_DIRECTORIES "${GLEW_ROOT}/include"
    )
    message(STATUS "GLEW: ${GLEW_LIB_PATH}")
else()
    find_package(GLEW REQUIRED)
    message(STATUS "GLEW (system): ${GLEW_LIBRARIES}")
endif()

# ── FreeGLUT ──────────────────────────────────────────────────
if (FREEGLUT_ROOT)
    find_library(FREEGLUT_LIB_PATH
        NAMES freeglut freeglut_static glut
        PATHS
            "${FREEGLUT_ROOT}/build/lib/Release"
            "${FREEGLUT_ROOT}/build/lib/Debug"
            "${FREEGLUT_ROOT}/build/lib"
            "${FREEGLUT_ROOT}/lib/x64"
            "${FREEGLUT_ROOT}/lib/Release/x64"
            "${FREEGLUT_ROOT}/lib"
        NO_DEFAULT_PATH
    )
    if (NOT FREEGLUT_LIB_PATH)
        message(FATAL_ERROR
            "在 FREEGLUT_ROOT='${FREEGLUT_ROOT}' 下找不到 freeglut.lib。\n"
            "请确认目录结构，例如：\n"
            "  ${FREEGLUT_ROOT}/lib/x64/freeglut.lib")
    endif()

    # 自编译包 include 可能在源码根或 build 下
    set(FREEGLUT_INC_CANDIDATES
        "${FREEGLUT_ROOT}/include"
        "${FREEGLUT_ROOT}/build/include"
    )
    set(FREEGLUT_INC_PATH "")
    foreach(_d ${FREEGLUT_INC_CANDIDATES})
        if (EXISTS "${_d}/GL/freeglut.h")
            set(FREEGLUT_INC_PATH "${_d}")
            break()
        endif()
    endforeach()
    if (NOT FREEGLUT_INC_PATH)
        message(FATAL_ERROR
            "找不到 freeglut.h，请确认 ${FREEGLUT_ROOT}/include/GL/freeglut.h 存在。")
    endif()

    add_library(FreeGLUT::freeglut UNKNOWN IMPORTED)
    set_target_properties(FreeGLUT::freeglut PROPERTIES
        IMPORTED_LOCATION             "${FREEGLUT_LIB_PATH}"
        INTERFACE_INCLUDE_DIRECTORIES "${FREEGLUT_INC_PATH}"
    )
    message(STATUS "FreeGLUT: ${FREEGLUT_LIB_PATH}")
else()
    find_package(FreeGLUT QUIET)
    if (NOT FreeGLUT_FOUND)
        find_library(FREEGLUT_LIB_PATH NAMES freeglut glut)
        find_path(FREEGLUT_INC_PATH NAMES GL/freeglut.h GL/glut.h)
        if (NOT FREEGLUT_LIB_PATH)
            message(FATAL_ERROR
                "找不到 FreeGLUT，请设置 FREEGLUT_ROOT 或系统环境变量 FREEGLUT_ROOT。")
        endif()
        add_library(FreeGLUT::freeglut UNKNOWN IMPORTED)
        set_target_properties(FreeGLUT::freeglut PROPERTIES
            IMPORTED_LOCATION             "${FREEGLUT_LIB_PATH}"
            INTERFACE_INCLUDE_DIRECTORIES "${FREEGLUT_INC_PATH}"
        )
        message(STATUS "FreeGLUT (auto): ${FREEGLUT_LIB_PATH}")
    endif()
endif()

# ── 可执行文件 ────────────────────────────────────────────────
add_executable(solar_test
    main_test.cpp
    core/application.cpp
    core/timer.cpp
    debug/gl_debug.cpp
    render/camera.cpp
    # 阶段 1 数学库 — 有 .cpp 时取消注释
    # math/vec3.cpp
    math/matrix4.cpp
)

target_include_directories(solar_test PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(solar_test PRIVATE
    OpenGL::GL
    GLEW::GLEW
    FreeGLUT::freeglut
)

# ── Windows：构建后自动拷贝运行时 DLL ────────────────────────
if (WIN32)
    if (GLEW_ROOT)
        set(GLEW_DLL "${GLEW_ROOT}/bin/Release/x64/glew32.dll")
        if (EXISTS "${GLEW_DLL}")
            add_custom_command(TARGET solar_test POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${GLEW_DLL}" $<TARGET_FILE_DIR:solar_test>
                COMMENT "Copying glew32.dll → output dir"
            )
        endif()
    endif()
    if (FREEGLUT_ROOT)
        # 自编译包：DLL 在 build/bin/Release 下
        set(FGLUT_DLL "${FREEGLUT_ROOT}/build/bin/Release/freeglut.dll")
        if (EXISTS "${FGLUT_DLL}")
            add_custom_command(TARGET solar_test POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${FGLUT_DLL}" $<TARGET_FILE_DIR:solar_test>
                COMMENT "Copying freeglut.dll → output dir"
            )
        endif()
    endif()
endif()

# ── 编译警告 ──────────────────────────────────────────────────
target_compile_options(solar_test PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /W4 /WX->
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
