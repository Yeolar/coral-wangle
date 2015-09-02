#
# Copyright (C) 2015, Yeolar
#

set(WANGLE_CODEC_SRCS
    LengthFieldBasedFrameDecoder.cpp
    LengthFieldPrepender.cpp
    LineBasedFrameDecoder.cpp
)
add_library(wangle_codec OBJECT ${WANGLE_CODEC_SRCS})


install(FILES
    ByteToMessageDecoder.h
    FixedLengthFrameDecoder.h
    LengthFieldBasedFrameDecoder.h
    LengthFieldPrepender.h
    LineBasedFrameDecoder.h
    MessageToByteEncoder.h
    StringCodec.h
    DESTINATION include/wangle/codec
)
