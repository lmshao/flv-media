//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#ifndef FLV_MEDIA_FLV_H
#define FLV_MEDIA_FLV_H

#include <cstdint>

struct FLVHeader {
    char signature[3] = {'F', 'L', 'V'}; // 'FLV'
    uint8_t version = 0x01;              // File version
    uint8_t flagVideo : 1;               // Video tags are present
    uint8_t : 1;                         // Must be 0
    uint8_t flagAudio : 1;               // Audio tags are present
    uint8_t : 5;                         // Must be 0
    uint32_t offset = 9;                 // value 9 for FLV version 1.

    FLVHeader() { *(&version + 1) = 0x00; }
    FLVHeader(bool hasVideo, bool hasAudio) {
        *(&version + 1) = 0x00;
        flagVideo = hasVideo ? 1 : 0;
        flagAudio = hasAudio ? 1 : 0;
    }
} __attribute__((packed));

enum TagType : uint8_t {
    TAG_AUDIO = 8,
    TAG_VIDEO = 9,
    TAG_SCRIPT = 18 // script data
};

struct FlvTagHeader {
    TagType type : 5;
    uint8_t filter : 1;   // Shall be 0 in unencrypted files, and 1 for encrypted tags
    uint8_t reserved : 2; // Reserved for FMS, should be 0
    uint8_t size[3]{};    // data size
    /// This value(milliseconds) is relative to the first tag in the FLV file,
    /// which always has a timestamp of 0
    uint8_t timestamp[3]{};
    uint8_t timestampExtended{};
    uint8_t streamId[3]{}; // Always 0
    uint8_t data[0];
};

#endif // FLV_MEDIA_FLV_H
