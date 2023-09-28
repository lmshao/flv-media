//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#ifndef FLV_MEDIA_AVC_CONFIGURATION_H
#define FLV_MEDIA_AVC_CONFIGURATION_H

#include <cstddef>
#include <cstdint>
#include <stdint.h>
#include <string>

class AVCConfiguration {
public:
    AVCConfiguration() = default;
    AVCConfiguration(const uint8_t *packet, size_t size);
    AVCConfiguration(const uint8_t *sps, size_t spsSize, const uint8_t *pps, size_t ppsSize);

    void SetSPS(const uint8_t *sps, size_t size);
    void SetPPS(const uint8_t *sps, size_t size);
    void SetConfigurationPacket(const uint8_t *pack, size_t size);

    std::string GetSPS() { return sps_; }
    std::string GetPPS() { return pps_; }
    std::string GetConfigurationPacket();
    int GetNALULengthSize() { return naluLengthSize_; }

private:
    bool ParsePacket();

private:
    std::string packet_;
    std::string sps_;
    std::string pps_;
    int naluLengthSize_ = 4;
};

#endif // FLV_MEDIA_AVC_CONFIGURATION_H