#
# Copyright (C) 2015, Yeolar
#

set(WANGLE_RX_SRCS
    Dummy.cpp
)
add_library(wangle_rx OBJECT ${WANGLE_RX_SRCS})


install(FILES
    Observable.h
    Observer.h
    Subject.h
    Subscription.h
    types.h
    DESTINATION include/wangle/deprecated/rx
)

