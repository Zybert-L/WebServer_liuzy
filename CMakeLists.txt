cmake_minimum_required(VERSION 3.10)

project(webserver)

set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE SOURCES "code/*.cpp")

add_executable(webserver ${SOURCES})

#不能是target_include_directories(webserver PRIVATE code/*.h),因为这里必须放路径

#如果想简化，可以这样：
#cmaketarget_include_directories(webserver PRIVATE code)
#但前提是你的代码里要写完整路径：
#include "Buffer/Buffer.h"
#include "config/config.h"  
#include "server/webserver.h"

target_include_directories(webserver PRIVATE 
    code
    code/Buffer
    code/config
    code/http
    code/Log
    code/Pool
    code/server
    code/timer
)

target_link_libraries(webserver pthread mysqlclient)

set_target_properties(webserver PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_custom_command(TARGET webserver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources ${CMAKE_SOURCE_DIR}/bin/resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/code/config/config.ini ${CMAKE_SOURCE_DIR}/bin/config.ini
    COMMENT "复制资源文件到bin目录"
)