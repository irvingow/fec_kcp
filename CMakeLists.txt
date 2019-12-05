cmake_minimum_required(VERSION 3.3)
project(fec_kcp)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -g")

include_directories(./include/)
include_directories(./lib/kcp/include)
include_directories(./lib/libfec/include)
include_directories(./lib/libfec/lib/include)

aux_source_directory(./source/ source_list)
aux_source_directory(./lib/kcp/source kcp_source_list)
aux_source_directory(./lib/libfec/source libfec_source_list)
aux_source_directory(./lib/libfec/lib/source libfec_lib_source_list)

link_libraries("glog")

#add_executable(udptun_client samples/udptun_client.cpp ${source_list})
#add_executable(udptun_server samples/udptun_server.cpp ${source_list})

file(GLOB_RECURSE mains RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/samples/*.cpp")
foreach(mainfile IN LISTS mains)
#     Get file name without directory
    get_filename_component(mainname ${mainfile} NAME_WE)
    add_executable(${mainname} ${mainfile} ${source_list} ${kcp_source_list} ${libfec_source_list} ${libfec_lib_source_list})
endforeach()

