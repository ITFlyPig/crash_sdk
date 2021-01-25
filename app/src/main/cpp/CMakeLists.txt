cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

#JNI 路径
set(JNI_PATH ${CMAKE_SOURCE_DIR})

# 其它动态库路径
# 动态库 路径
set(SHARED_PATH ${CMAKE_SOURCE_DIR}/../jniLibs/${CMAKE_ANDROID_ARCH_ABI})


include_directories(${JNI_PATH}/include)


#jni cpp
FILE(GLOB JNI_ALL_CPP ${JNI_PATH}/*.cpp)
FILE(GLOB JNI_ALL_H ${JNI_PATH}/*.h)

#-------------------------------------------------加载库文件----------------------------------------------


add_library(unwind SHARED IMPORTED)
set_target_properties(unwind PROPERTIES IMPORTED_LOCATION ${SHARED_PATH}/libunwind.so)

add_library(unwind-aarch64 SHARED IMPORTED)
set_target_properties(unwind-aarch64 PROPERTIES IMPORTED_LOCATION ${SHARED_PATH}/libunwind-aarch64.so)

add_library(unwind-ptrace SHARED IMPORTED)
set_target_properties(unwind-ptrace PROPERTIES IMPORTED_LOCATION ${SHARED_PATH}/libunwind-ptrace.so)

add_library(unwind-setjmp SHARED IMPORTED)
set_target_properties(unwind-setjmp PROPERTIES IMPORTED_LOCATION ${SHARED_PATH}/libunwind-setjmp.so)

add_library(
        native-lib
        SHARED
        ${JNI_ALL_CPP}
)


find_library(
        log-lib
        log
)


#AVTools
target_link_libraries(
        native-lib
        unwind
        unwind-aarch64
        unwind-ptrace
        unwind-setjmp
        ${log-lib}
)
