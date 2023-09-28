//
// Copyright 2023 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef FLV_AUDIO_SPECIFIC_CONFIG_H
#define FLV_AUDIO_SPECIFIC_CONFIG_H

#include <string>

/// [Audio Specific Config](https://wiki.multimedia.cx/index.php?title=MPEG-4_Audio)

enum AudioObjectType {
    AAC_MAIN = 1,     /// AAC Main
    AAC_LC,           /// AAC LC (Low Complexity)
    AAC_SSR,          /// AAC SSR (Scalable Sample Rate)
    AAC_LTP,          /// AAC LTP (Long Term Prediction)
    AAC_SBR,          /// SBR (Spectral Band Replication)
    AAC_SCALABLE,     /// AAC Scalable
    TWIN_VQ,          /// TwinVQ
    CELP,             /// CELP (Code Excited Linear Prediction)
    HXVC,             /// HXVC (Harmonic Vector eXcitation Coding)
    TTSI = 12,        /// TTSI (Text-To-Speech Interface)
    MAIN,             /// Main Synthesis
    WAVETABLE,        /// Wavetable Synthesis
    MIDI,             /// General MIDI
    ASAE,             /// Algorithmic Synthesis and Audio Effects
    ER_AAC_LC,        /// ER (Error Resilient) AAC LC
    ER_AAC_LTP = 19,  /// ER AAC LTP
    ER_AAC_SCALABLE,  /// ER AAC Scalable
    ER_TWIN_VQ,       /// ER TwinVQ
    ER_BSAC,          /// ER BSAC (Bit-Sliced Arithmetic Coding)
    ER_AAC_LD,        /// ER AAC LD (Low Delay)
    ER_CELP,          /// ER CELP
    ER_HVXC,          /// ER HVXC
    ER_HILN,          /// ER HILN (Harmonic and Individual Lines plus Noise)
    ER_PARAMETRIC,    /// ER Parametric
    SSC,              /// SSC (SinuSoidal Coding)
    PS,               /// PS (Parametric Stereo)
    MPEG_SURROUND,    /// MPEG Surround
    ESCAPE_VALUE,     /// (Escape value)
    LAYER_1,          /// Layer-1
    LAYER_2,          /// Layer-2
    LAYER_3,          /// Layer-3
    DST,              /// DST (Direct Stream Transfer)
    ALS,              /// ALS (Audio Lossless)
    SLS,              /// SLS (Scalable LosslesS)
    SLS_NON,          /// SLS non-core
    ER_AAC_ELD,       /// ER AAC ELD (Enhanced Low Delay)
    SMR,              /// SMR (Symbolic Music Representation) Simple
    SMR_MAIN,         /// SMR Main
    USAC_NO_SBR,      /// USAC (Unified Speech and Audio Coding) (no SBR)
    SAOC,             /// SAOC (Spatial Audio Object Coding)
    LD_MPEG_SURROUND, /// LD MPEG Surround
    USAC = 45,        /// USAC
};

// The Audio Specific Config is the global header for MPEG-4 Audio
//
// 5 bits: object type
// if (object type == 31)
//     6 bits + 32: object type
// 4 bits: frequency index
// if (frequency index == 15)
//     24 bits: frequency
// 4 bits: channel configuration
// var bits: AOT Specific Config
class AudioSpecificConfig {
public:
    AudioSpecificConfig(char *data, int size) {
        data_.assign(data, size);
        printf("%02x-%02x\n", data[0] & 0xff, data[1] & 0xff);
    }

    int GetObjectType() {
        if (!objectType_) {
            Parse();
        }
        return objectType_;
    }

    int GetSampleRate() {
        if (!samplingRate_) {
            Parse();
        }
        return samplingRate_;
    }

    int GetChannels() {
        if (!channels_) {
            Parse();
        }
        return channels_;
    }

    bool Parse() {
        if (data_.size() < 2) {
            printf("Audio specific config requires at least 2 bytes of data");
            return false;
        }

        static int SamplingRate[15] = {96000, 88200, 64000, 48000, 44100, 32000, 240000, 22050,
                                       16000, 12000, 11025, 8000,  7350,  0,     0};

        int freqIndex;
        // 1111'1000-0000'0000
        objectType_ = (data_[0] >> 3) & 0x1f;
        if (objectType_ != 31) {
            // 0000'0111-1000'0000
            freqIndex = ((data_[0] & 0x07) << 1) | ((data_[1] >> 7) & 0x01);
            if (freqIndex != 15) {
                printf("freq: %d\n", freqIndex);
                samplingRate_ = SamplingRate[freqIndex];
                // 0000'0000-0111'1000
                channels_ = (data_[1] >> 3) & 0x0f;
            } else {
                if (data_.size() < 5) {
                    printf("Audio specific config requires at least 5 bytes of data");
                    return false;
                }
                // 0000'0000-0111'1111-1111'1111-1111'1111-1000'0000
                samplingRate_ = ((data_[1] & 0x7f) << 17) | (data_[2] << 9) | (data_[3] << 1) | (data_[4] >> 7) & 0x01;
                // 0000'0000-00000'0000-0000'0000-0000'0000-0111'1000
                channels_ = (data_[4] >> 3) & 0x0f;
            }
        } else {
            // 0000'0111-1110'0000
            objectType_ = 32 + (((data_[0] & 0x07) << 3) | (data_[1] >> 5) & 0x07);
            // 0000'0000-0001'1110
            freqIndex = (data_[1] >> 1) & 0x0f;
            if (freqIndex != 15) {
                if (data_.size() < 3) {
                    printf("Audio specific config requires at least 3 bytes of data");
                    return false;
                }
                samplingRate_ = SamplingRate[freqIndex];

                // 0000'0000-0000'0001-1110‘0000
                channels_ = (data_[1] & 0x01) << 3 | (data_[2] >> 5) & 0x07;
            } else {
                if (data_.size() < 6) {
                    printf("Audio specific config requires at least 3 bytes of data");
                    return false;
                }

                // 0000'0000-0000'0001-1111’1111-1111‘1111-1111’1110
                samplingRate_ = ((data_[1] & 0x01) << 23) | (data_[2] << 15) | (data_[3] << 7) | (data_[4] >> 1) & 0x7f;
                // 0000'0000-0000'0000-0000’0000-0000‘0000-0000’0001-1110'0000
                channels_ = ((data_[4] & 0x01) << 3) | ((data_[5] >> 5) & 0x07);
            }
        }

        if (channels_ == 7) {
            channels_ = 8;
        }
        return true;
    }

private:
    std::string data_;
    int objectType_ = 0;
    int samplingRate_ = 0;
    int channels_ = 0;
};

#endif // FLV_AUDIO_SPECIFIC_CONFIG_H