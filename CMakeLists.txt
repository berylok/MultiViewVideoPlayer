cmake_minimum_required(VERSION 3.16)
project(MultiViewMediaPlayer3 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia MultimediaWidgets)

# FFmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(SWSCALE REQUIRED libswscale)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWRESAMPLE REQUIRED libswresample)

# 源文件（使用正斜杠，兼容所有平台）
set(SOURCES
    src/main.cpp
    src/decoderthread.cpp
    src/videorenderwidget.cpp
    src/multiviewplayer.cpp
)

set(HEADERS
    src/decoderthread.h
    src/videorenderwidget.h
    src/multiviewplayer.h
)

# 创建可执行文件
add_executable(MultiViewMediaPlayer3 ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(MultiViewMediaPlayer3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src  # 添加 src 目录到包含路径
    ${AVCODEC_INCLUDE_DIRS}
    ${AVFORMAT_INCLUDE_DIRS}
    ${SWSCALE_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${SWRESAMPLE_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(MultiViewMediaPlayer3 PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${AVCODEC_LIBRARIES}
    ${AVFORMAT_LIBRARIES}
    ${SWSCALE_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SWRESAMPLE_LIBRARIES}
)

# 设置编译选项
target_compile_options(MultiViewMediaPlayer3 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# 安装
install(TARGETS MultiViewMediaPlayer3
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)