
cmake_minimum_required(VERSION 3.4.1)
#设置第三方文件的路径
set(libs_DIR ${CMAKE_SOURCE_DIR}/../../../libs)

file(GLOB native_srcs "${CMAKE_SOURCE_DIR}/*.cpp")
add_library( # Sets the name of the library.
        saozhu

        SHARED
        #        native-lib.cpp
        ${native_srcs})

find_library( # Sets the name of the path variable.
        log-lib

        log)
#引入头文件
include_directories(${CMAKE_SOURCE_DIR}/include)

#------------------编译ffmpeg静态文件------------------
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${libs_DIR}/${ANDROID_ABI}")

#------------------把所有库文件都引入工程------------------
target_link_libraries( # Specifies the target library.
        saozhu
        avfilter
        avformat
        avcodec
        avutil
        swresample
        swscale
        android
        z
        OpenSLES
        ${log-lib})