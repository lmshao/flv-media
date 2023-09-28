//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#ifndef FLV_MEDIA_ADTS_HEADER_H
#define FLV_MEDIA_ADTS_HEADER_H

#include <cstdint>
#include <cstring>

static int sampling_frequency_table[13] = {96000, 88200, 64000, 48000, 44100, 32000, 24000,
                                           22050, 16000, 12000, 11025, 8000,  7350};
// 7 bytes
struct ADTSHeader {
    // fixed header
    // 1 byte
    uint8_t sync_word_l : 8; // 0xFF
    // 2 byte
    // sync_word_h + id + layer + protection_absent
    uint8_t protection_absent : 1; // 1 no CRC, 0 has CRC
    uint8_t layer : 2;             // 00
    uint8_t id : 1;                // 0: MPEG-4, 1: MPEG-2
    uint8_t sync_word_h : 4;       // 0xF

    // 3rd byte
    // profile + sampling_frequency_index + private_bit + channel_configuration_l
    uint8_t channel_configuration_l : 1;
    uint8_t private_bit : 1;
    uint8_t sampling_frequency_index : 4;
    uint8_t profile : 2; // 0:main, 1: LC, 2: SSR, 3: reserved

    // 4th byte
    uint8_t aac_frame_length_l : 2;
    uint8_t copyright_identification_start : 1;
    uint8_t copyright_identification_bit : 1;
    uint8_t home : 1;
    uint8_t original_copy : 1;
    uint8_t channel_configuration_h : 2;

    // 5th byte
    uint8_t aac_frame_length_m : 8;
    // 6th byte
    uint8_t adts_buffer_fullness_l : 5;
    uint8_t aac_frame_length_h : 3;
    // 7th byte
    uint8_t number_of_raw_data_blocks_in_frame : 2;
    uint8_t adts_buffer_fullness_h : 6; // adts_buffer_fullness 0x7ff VBR

    ADTSHeader() {
        memset(this, 0, sizeof(ADTSHeader));
        SetSyncWord();
        protection_absent = 1;
        profile = 1;
    }

    ADTSHeader(int samplingFreq, int channel, int length) {
        memset(this, 0, sizeof(ADTSHeader));
        SetSyncWord();
        SetSamplingFrequency(samplingFreq);
        SetChannel(channel);
        SetLength(length);
        protection_absent = 1;
        profile = 1;
    }

    ADTSHeader &SetSyncWord() {
        sync_word_l = 0xff;
        sync_word_h = 0xf;
        return *this;
    }

    ADTSHeader &SetSamplingFrequency(int sf) {
        sampling_frequency_index = 0xf;
        for (int i = 0; i < 13; ++i) {
            if (sampling_frequency_table[i] == sf) {
                sampling_frequency_index = i;
                break;
            }
        }
        return *this;
    }

    ADTSHeader &SetChannel(int ch) {
        if (ch > 0 && ch < 7) {
            channel_configuration_h = ch;
        } else if (ch == 8) {
            channel_configuration_h = 7 >> 24;
        }
        return *this;
    }

    // length = header length + aac es stream length
    ADTSHeader &SetLength(int length) {
        aac_frame_length_l = (length >> 11) & 0x03;
        aac_frame_length_m = (length >> 3) & 0xff;
        aac_frame_length_h = (length & 0x07);
        return *this;
    }

    ADTSHeader &SetVBR() {
        adts_buffer_fullness_l = 0b11111;
        adts_buffer_fullness_h = 0b111111;
        return *this;
    }

    int GetLength() const {
        int l = (aac_frame_length_l << 11) | (aac_frame_length_m << 3) | (aac_frame_length_h);
        return l;
    }

    int GetChannel() const {
        if (channel_configuration_h == 8) {
            return 8;
        }
        return channel_configuration_h;
    }

    int GetFrequency() const { return sampling_frequency_table[sampling_frequency_index]; }
};

#endif // FLV_MEDIA_ADTS_HEADER_H