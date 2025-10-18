#顶层Cmake，设置全局链接库以及添加子目录，和项目的头文件.h位置
#设置最低版本和项目名称
cmake_minimum_required(VERSION 3.0)
#获取当前目录下的所有源文件项目
file(GLOB SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/*cc)
list(REMOVE_ITEM SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/main.cc)


# 创建共享库
add_library(src_lib SHARED ${SRC_FILE})

#创建可执行文件
add_executable(main  main.cc)

#链接必要的库
target_link_libraries(main src_lib memory_lib log_lib ${LIBS})