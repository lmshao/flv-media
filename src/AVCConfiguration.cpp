//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#include "AVCConfiguration.h"
#include <assert.h>
#include <cstdint>
#include <cstdio>
#include <vector>

/*
 * AVCDecoderConfigurationRecord format specification
 * bits
 * 8   version ( always 0x01 )
 * 8   avc profile ( sps[0][1] )
 * 8   avc compatibility ( sps[0][2] )
 * 8   avc level ( sps[0][3] )
 * 6   reserved ( all bits on )
 * 2   NALULengthSizeMinusOne
 * 3   reserved ( all bits on )
 * 5   number of SPS NALUs (usually 1)
 *         repeated once per SPS:
 * 16  SPS size
 *         variable SPS NALU data
 * 8   number of PPS NALUs (usually 1)
 *         repeated once per PPS
 * 16  PPS size
 *         variable PPS NALU data
 */

AVCConfiguration::AVCConfiguration(const uint8_t *packet, size_t size) {
    SetConfigurationPacket(packet, size);
}

AVCConfiguration::AVCConfiguration(const uint8_t *sps, size_t spsSize, const uint8_t *pps, size_t ppsSize) {
    SetSPS(sps, spsSize);
    SetPPS(pps, ppsSize);
}

void AVCConfiguration::SetSPS(const uint8_t *sps, size_t size) {
    sps_ = std::string((const char *)sps, size);
}

void AVCConfiguration::SetPPS(const uint8_t *pps, size_t size) {
    pps_ = std::string((const char *)pps, size);
}

void AVCConfiguration::SetConfigurationPacket(const uint8_t *pack, size_t size) {
    packet_ = std::string((const char *)pack, size);
    ParsePacket();
}

std::string AVCConfiguration::GetConfigurationPacket() {
    if (!packet_.empty()) {
        return packet_;
    }

    if (sps_.size() < 4 || pps_.empty()) {
        return {};
    }

    std::string packet;

    char config[6] = {};
    config[0] = 0x01;           // version
    config[1] = sps_.data()[1]; // profileIndication: Baseline profile 66, Main profile 77, High profile 100
    config[2] = sps_.data()[2]; // profileCompatibility
    config[3] = sps_.data()[3]; // levelIndication
    config[4] = 0xff;
    uint8_t numOfSPS = 1;
    config[5] = 0b1110000 | numOfSPS;

    packet.assign(config, 6);

    uint16_t spsLength = (uint16_t)sps_.size();
    packet.push_back((spsLength >> 8) & 0xff);
    packet.push_back(spsLength & 0xff);
    packet.append(sps_);

    uint8_t numOfPPS = 1;
    packet.push_back(numOfPPS);
    uint16_t ppsLength = (uint16_t)pps_.size();
    packet.push_back((ppsLength >> 8) & 0xff);
    packet.push_back(ppsLength & 0xff);
    packet.append(pps_);

    packet_ = std::move(packet);
    return packet_;
}

bool AVCConfiguration::ParsePacket() {
    if (packet_.empty()) {
        return false;
    }

    assert(packet_.size() > 8);
    auto p = packet_.data();

    assert(p[0] == 0x01); // version

    naluLengthSize_ = (p[4] & 0x03) + 1;
    int numOfSPS = p[5] & 0x1f;
    assert(numOfSPS == 1);

    p += 6;
    int spsLength = (p[0] << 8) | p[1];
    p += 2;
    assert(p < packet_.data() + packet_.size());

    sps_.assign(p, spsLength);
    p += spsLength;

    int numOfPPS = p[0];
    assert(numOfPPS == 1);
    p += 1;
    int ppsLength = (p[0] << 8) | p[1];
    p += 2;
    assert(p < packet_.data() + packet_.size());

    pps_.assign(p, ppsLength);

    return true;
}
