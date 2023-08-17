//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#ifndef FLV_MEDIA_VIDEO_TAG_H
#define FLV_MEDIA_VIDEO_TAG_H

#include <cstdint>

enum VideoFrameType : uint8_t {
    KEY_FRAME = 1,          // keyframe (for AVC, a seekable frame)
    INTER_FRAME,            // inter frame (for AVC, a non-seekable frame)
    DISPOSABLE_INTER_FRAME, // disposable inter frame (H.263 only)
    GENERATED_KEY_FRAME,    // generated keyframe (reserved for server use only)
    INFO_FRAME,             // video info/command frame
};

enum VideoCodec : uint8_t {
    CODEC_JPEG = 1,        // JPEG (currently unused)
    CODEC_SORENSON_H263,   // Sorenson H.263
    CODEC_SCREEN_VIDEO,    // Screen video
    CODEC_VP6,             // On2 VP6
    CODEC_VP6_ALPHA,       // On2 VP6 with alpha channel
    CODEC_SCREEN_VIDEO_V2, // Screen video version 2
    CODEC_AVC              // H.264/AVC
};

enum AVCPacketType : uint8_t {
    AVC_HEADER = 0, // AVC sequence header
    AVC_NALU,       // AVC NALU
    AVC_END         // AVC end of sequence (lower level NALU sequence ender is not required or supported)
};

struct VideoTagHeader {
    VideoCodec codec : 4;
    VideoFrameType frameType : 4;
};

struct AVCVideoTagHeader {
    VideoCodec codec : 4;
    VideoFrameType frameType : 4; // 7 for AVC
    AVCPacketType packetType;
    /// if AVCPacketType == 1, Composition time offset, else 0
    uint8_t cst[3];
    /// video composition time(PTS - DTS), AVC/HEVC/AV1 only
    uint8_t data[0];
};

#endif // FLV_MEDIA_VIDEO_TAG_H
