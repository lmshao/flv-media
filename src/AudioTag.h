//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#ifndef FLV_MEDIA_AUDIO_TAG_H
#define FLV_MEDIA_AUDIO_TAG_H

#include <cstdint>

enum AudioCodec : uint8_t {
    CODEC_LPCM_PE = 0,    //< Linear PCM, platform endian
    CODEC_ADPCM,          // ADPCM
    CODEC_MP3,            // MP3
    CODEC_LPCM_LE,        // Linear PCM, little endian
    CODEC_NELLYMOSER_16K, // Nelly Moser 16 kHz mono
    CODEC_NELLYMOSER_8K,  // Nelly Moser 8 kHz mono
    CODEC_NELLYMOSER,     // Nelly Moser
    CODEC_G711_A,         // G.711 A-law logarithmic PCM
    CODEC_G711_U,         // G.711 mu-law logarithmic PCM
    CODEC_RESERVED,       // Reserved
    CODEC_AAC = 10,       // AAC
    CODEC_SPEEX,          // Speex
    CODEC_MP3_8K = 14,    // MP3 8-Khz
    CODEC_DEVICE_SPECIFIC // Device-specific sound
};

enum AudioSamplingRate : uint8_t {
    SR_5500 = 0,
    SR_11000,
    SR_22000,
    SR_44000 // For AAC: always 3
};

enum AudioSamplingBitDepth : uint8_t {
    SBD_8 = 0, // 8 bits
    SBD_16     // 16 bits
};

enum AudioChannel : uint8_t {
    CHANNEL_MONO = 0, // For Nelly Moser: always 0
    CHANNEL_STEREO    // For AAC: always 1
};

enum AACPacketType : uint8_t {
    AAC_HEADER = 0, // 0: AAC sequence header, AudioSpecificConfig
    AAC_RAW_DATA    // 1: Raw AAC frame data
};

struct AudioTagHeader {
    AudioCodec codec;
    AudioSamplingRate rate;
    AudioSamplingBitDepth bits;
    AudioChannel channels;
    uint8_t data[0];
};

struct AACAudioTagHeader {
    AudioCodec codec;
    AudioSamplingRate rate;
    AudioSamplingBitDepth bits;
    AudioChannel channels;
    AACPacketType packetType;
    uint8_t data[0];
};

#endif // FLV_MEDIA_AUDIO_TAG_H
