cmake_minimum_required(VERSION 3.22.1)
project(terminal C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(android_native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)
target_include_directories(android_native_app_glue PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue
)

add_library(terminal SHARED main.c)
target_include_directories(terminal PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)
target_link_libraries(terminal
    android_native_app_glue
    EGL
    GLESv3
    android
    log
)
