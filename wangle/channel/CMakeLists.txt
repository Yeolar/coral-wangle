#
# Copyright (C) 2015, Yeolar
#

set(WANGLE_CHANNEL_SRCS
    FileRegion.cpp
    Pipeline.cpp
)
add_library(wangle_channel OBJECT ${WANGLE_CHANNEL_SRCS})


install(FILES
    AsyncSocketHandler.h
    EventBaseHandler.h
    FileRegion.h
    HandlerContext.h
    HandlerContext-inl.h
    Handler.h
    OutputBufferingHandler.h
    Pipeline.h
    Pipeline-inl.h
    StaticPipeline.h
    DESTINATION include/wangle/channel
)
install(FILES
    broadcast/BroadcastHandler.h
    broadcast/BroadcastHandler-inl.h
    broadcast/BroadcastPool.h
    broadcast/BroadcastPool-inl.h
    broadcast/ObservingHandler.h
    broadcast/ObservingHandler-inl.h
    broadcast/Subscriber.h
    DESTINATION include/wangle/channel/broadcast
)
