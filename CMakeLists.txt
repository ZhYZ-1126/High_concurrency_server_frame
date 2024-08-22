cmake_minimum_required(VERSION 2.8)
project(HCServer)

include(cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(LIB_SRC
    HCServer/address.cc
    HCServer/bytearray.cc
    HCServer/config.cc
    HCServer/fiber.cc
    HCServer/fd_manager.cc
    HCServer/hook.cc
    HCServer/http/http_connection.cc
    HCServer/http/http.cc
    HCServer/http/http_parser.cc
    HCServer/http/http_session.cc
    HCServer/http/http_server.cc
    HCServer/http/httpclient_parser.rl.cc
    HCServer/http/http11_parser.rl.cc
    HCServer/http/servlet.cc
    HCServer/iomanager.cc
    HCServer/log.cc
    HCServer/mutex.cc
    HCServer/scheduler.cc
    HCServer/socket.cc
    HCServer/stream.cc
    HCServer/streams/socket_stream.cc
    HCServer/tcp_server.cc
    HCServer/thread.cc
    HCServer/timer.cc
    HCServer/uri.cc
    HCServer/util.cc
    )

ragelmaker(HCServer/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/HCServer/http)
ragelmaker(HCServer/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/HCServer/http)

ragelmaker(HCServer/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/HCServer)



add_library(HCServer SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(HCServer) #_FILE_
#add_library(HCServer_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (HCServer_static PROPERTIES OUTPUT_NAME "HCServer")

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)


set(LIBS
    HCServer
    dl
    pthread
    yaml-cpp
    )
message("***",${LIBS})

add_executable(test test/test.cc)
add_dependencies(test HCServer)
force_redefine_file_macro_for_sources(test) #_FILE_
target_link_libraries(test ${LIBS})

add_executable(test_config test/test_config.cc)
add_dependencies(test_config HCServer)
force_redefine_file_macro_for_sources(test_config) #_FILE_
target_link_libraries(test_config ${LIBS})

add_executable(test_thread test/test_thread.cc)
add_dependencies(test_thread HCServer)
force_redefine_file_macro_for_sources(test_thread) #_FILE_
target_link_libraries(test_thread ${LIBS})

add_executable(test_util test/test_util.cc)
add_dependencies(test_util HCServer)
force_redefine_file_macro_for_sources(test_util) #_FILE_
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber test/test_fiber.cc)
add_dependencies(test_fiber HCServer)
force_redefine_file_macro_for_sources(test_fiber) #_FILE_
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler test/test_scheduler.cc)
add_dependencies(test_scheduler HCServer)
force_redefine_file_macro_for_sources(test_scheduler) #_FILE_
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager test/test_iomanager.cc)
add_dependencies(test_iomanager HCServer)
force_redefine_file_macro_for_sources(test_iomanager) #_FILE_
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook test/test_hook.cc)
add_dependencies(test_hook HCServer)
force_redefine_file_macro_for_sources(test_hook) #_FILE_
target_link_libraries(test_hook ${LIBS})

add_executable(test_address test/test_address.cc)
add_dependencies(test_address HCServer)
force_redefine_file_macro_for_sources(test_address) #_FILE_
target_link_libraries(test_address ${LIBS})

add_executable(test_socket test/test_socket.cc)
add_dependencies(test_socket HCServer)
force_redefine_file_macro_for_sources(test_socket) #_FILE_
target_link_libraries(test_socket ${LIBS})

add_executable(test_bytearray test/test_bytearray.cc)
add_dependencies(test_bytearray HCServer)
force_redefine_file_macro_for_sources(test_bytearray) #_FILE_
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_http test/test_http.cc)
add_dependencies(test_http HCServer)
force_redefine_file_macro_for_sources(test_http) #_FILE_
target_link_libraries(test_http ${LIBS})

add_executable(test_parser test/test_parser.cc)
add_dependencies(test_parser HCServer)
force_redefine_file_macro_for_sources(test_parser) #_FILE_
target_link_libraries(test_parser ${LIBS})

add_executable(test_tcp_server test/test_tcp_server.cc)
add_dependencies(test_tcp_server HCServer)
force_redefine_file_macro_for_sources(test_tcp_server) #_FILE_
target_link_libraries(test_tcp_server ${LIBS})

add_executable(echo_server examples/echo_server.cc)
add_dependencies(echo_server HCServer)
force_redefine_file_macro_for_sources(echo_server) #_FILE_
target_link_libraries(echo_server ${LIBS})

add_executable(test_http_server test/test_http_server.cc)
add_dependencies(test_http_server HCServer)
force_redefine_file_macro_for_sources(test_http_server) #_FILE_
target_link_libraries(test_http_server ${LIBS})

add_executable(test_http_connection test/test_http_connection.cc)
add_dependencies(test_http_connection HCServer)
force_redefine_file_macro_for_sources(test_http_connection) #_FILE_
target_link_libraries(test_http_connection ${LIBS})

add_executable(test_uri test/test_uri.cc)
add_dependencies(test_uri HCServer)
force_redefine_file_macro_for_sources(test_uri) #_FILE_
target_link_libraries(test_uri ${LIBS})



# add_executable(TEST test/TEST.cc)
# add_dependencies(TEST HCServer)
# target_link_libraries(TEST HCServer)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)