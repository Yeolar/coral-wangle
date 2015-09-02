#
# Copyright (C) 2015, Yeolar
#

set(WANGLE_ACCEPTOR_SRCS
    Acceptor.cpp
    AcceptorHandshakeHelper.cpp
    ConnectionManager.cpp
    LoadShedConfiguration.cpp
    ManagedConnection.cpp
    SocketOptions.cpp
    TransportInfo.cpp
)
add_library(wangle_acceptor OBJECT ${WANGLE_ACCEPTOR_SRCS})


install(FILES
    Acceptor.h
    AcceptorHandshakeHelper.h
    ConnectionCounter.h
    ConnectionManager.h
    DomainNameMisc.h
    LoadShedConfiguration.h
    ManagedConnection.h
    NetworkAddress.h
    PeekingAcceptorHandshakeHelper.h
    SecureTransportType.h
    ServerSocketConfig.h
    SocketOptions.h
    TransportInfo.h
    DESTINATION include/wangle/acceptor
)
