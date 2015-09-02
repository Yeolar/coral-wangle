#
# Copyright (C) 2015, Yeolar
#

set(WANGLE_BOOTSTRAP_SRCS
    ServerBootstrap.cpp
)
add_library(wangle_bootstrap OBJECT ${WANGLE_BOOTSTRAP_SRCS})


install(FILES
    AcceptRoutingHandler.h
    AcceptRoutingHandler-inl.h
    ClientBootstrap.h
    RoutingDataHandler.h
    RoutingDataHandler-inl.h
    ServerBootstrap.h
    ServerBootstrap-inl.h
    ServerSocketFactory.h
    DESTINATION include/wangle/bootstrap
)
