#
# Copyright (C) 2015, Yeolar
#

set(WANGLE_CONCURRENT_SRCS
    Codel.cpp
    CPUThreadPoolExecutor.cpp
    GlobalExecutor.cpp
    IOThreadPoolExecutor.cpp
    ThreadPoolExecutor.cpp
)
add_library(wangle_concurrent OBJECT ${WANGLE_CONCURRENT_SRCS})


install(FILES
    BlockingQueue.h
    Codel.h
    CPUThreadPoolExecutor.h
    FiberIOExecutor.h
    FutureExecutor.h
    GlobalExecutor.h
    IOExecutor.h
    IOObjectCache.h
    IOThreadPoolExecutor.h
    LifoSemMPMCQueue.h
    NamedThreadFactory.h
    PriorityLifoSemMPMCQueue.h
    PriorityThreadFactory.h
    ThreadFactory.h
    ThreadPoolExecutor.h
    DESTINATION include/wangle/concurrent
)
