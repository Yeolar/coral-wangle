#
# Copyright (C) 2015, Yeolar
#

add_executable(wangle_example_accept_steering
    accept_steering/accept_steering_server.cpp)
target_link_libraries(wangle_example_accept_steering wangle_static)


add_executable(wangle_example_file
    file/FileServer.cpp)
target_link_libraries(wangle_example_file wangle_static)


add_executable(wangle_example_proxy
    proxy/Proxy.cpp)
target_link_libraries(wangle_example_proxy wangle_static)


#add_executable(wangle_example_rpc_client
#    rpc/RpcClient.cpp)
#target_link_libraries(wangle_example_rpc_client wangle_static)
#
#
#add_executable(wangle_example_rpc_server
#    rpc/RpcServer.cpp)
#target_link_libraries(wangle_example_rpc_server wangle_static)
#

add_executable(wangle_example_telnet_client
    telnet/TelnetClient.cpp)
target_link_libraries(wangle_example_telnet_client wangle_static)


add_executable(wangle_example_telnet_server
    telnet/TelnetServer.cpp)
target_link_libraries(wangle_example_telnet_server wangle_static)

