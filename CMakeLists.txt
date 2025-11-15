cmake_minimum_required(VERSION 3.10)
project(nmcli-alt VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找所需的包
find_package(PkgConfig REQUIRED)

# 查找libnl库
pkg_check_modules(LIBNL REQUIRED libnl-3.0)
pkg_check_modules(LIBNL_ROUTE REQUIRED libnl-route-3.0)
pkg_check_modules(LIBNL_GENL REQUIRED libnl-genl-3.0)
pkg_check_modules(SDBUSCPP REQUIRED sdbus-c++)

# 包含头文件目录
include_directories(${LIBNL_INCLUDE_DIRS})
include_directories(${LIBNL_ROUTE_INCLUDE_DIRS})
include_directories(${LIBNL_GENL_INCLUDE_DIRS})
include_directories(${SDBUSCPP_INCLUDE_DIRS})
include_directories(include)

# 链接库目录
link_directories(${LIBNL_LIBRARY_DIRS})
link_directories(${LIBNL_ROUTE_LIBRARY_DIRS})
link_directories(${LIBNL_GENL_LIBRARY_DIRS})
link_directories(${SDBUSCPP_LIBRARY_DIRS})

# 添加可执行文件
add_executable(nmcli-alt
    src/main.cpp
    src/network_manager.cpp
    src/iwd_manager.cpp
    src/station.cpp
    src/process_util.cpp
)

# 链接库
target_link_libraries(nmcli-alt
    ${LIBNL_LIBRARIES}
    ${LIBNL_ROUTE_LIBRARIES}
    ${LIBNL_GENL_LIBRARIES}
    ${SDBUSCPP_LIBRARIES}
)

# 安装规则
install(TARGETS nmcli-alt DESTINATION bin)
