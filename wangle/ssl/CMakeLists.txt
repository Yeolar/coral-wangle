#
# Copyright (C) 2015, Yeolar
#

set(WANGLE_SSL_SRCS
    PasswordInFile.cpp
    SSLContextManager.cpp
    SSLSessionCacheManager.cpp
    SSLUtil.cpp
    TLSTicketKeyManager.cpp
)
add_library(wangle_ssl OBJECT ${WANGLE_SSL_SRCS})


install(FILES
    ClientHelloExtStats.h
    DHParam.h
    PasswordInFile.h
    SSLCacheOptions.h
    SSLCacheProvider.h
    SSLContextConfig.h
    SSLContextManager.h
    SSLSessionCacheManager.h
    SSLStats.h
    SSLUtil.h
    TLSTicketKeyManager.h
    TLSTicketKeySeeds.h
    DESTINATION include/wangle/ssl
)
