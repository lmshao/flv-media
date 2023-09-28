//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#include "ADTSHeader.h"
#include "AMF.h"
#include "AVCConfiguration.h"
#include "AudioSpecificConfig.h"
#include "AudioTag.h"
#include "FLV.h"
#include "File.h"
#include "VideoTag.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <functional>
#include <getopt.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void ShowUsage(char *exe) {
    printf("Usage:\n%s -i <file.flv> -m <video.h264,audio.aac> -d <file.flv> -h\n", exe);
    printf("\t-i info *.flv\n");
    printf("\t-m mux (*.h264,*.aac -> *.flv)\n");
    printf("\t-d demux (*.flv -> *.h264)\n");
    printf("\t-h help\n");
}

bool ProcessArgs(int argc, char *argv[], char &operation, char *&file) {
    int ret = getopt(argc, argv, ":i:m:d:h");
    switch (ret) {
        case ('i'):
            operation = 'i';
            file = optarg;
            break;
        case ('m'):
            operation = 'm';
            file = optarg;
            break;
        case ('d'):
            operation = 'd';
            file = optarg;
            break;
        case ':':
            printf("option [-%c] requires an argument\n", (char)optopt);
            break;
        case '?':
            printf("unknown option: %c\n", (char)optopt);
            break;
        default:
            break;
    }

    if (operation == 0) {
        ShowUsage(argv[0]);
        return false;
    }
    return true;
}

#define ASSERT_LOG_RETURN(exp, msg)                                                                                    \
    do {                                                                                                               \
        if (!(exp)) {                                                                                                  \
            printf("ERROR: %s\n", msg);                                                                                \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)

using Callback = std::function<void(const uint8_t *data, size_t size)>;

void ParseFlvFile(const uint8_t *data, size_t size, const Callback &videoCallback, const Callback &audioCallback) {
    auto p = data;
    auto end = data + size;
    AVCConfiguration config;
    ADTSHeader adtsHeader;
    int avcCNALULengthSize = 4;
    int preTagSizeLength = 4;

    assert(size > sizeof(FLVHeader));

    ASSERT_LOG_RETURN(p[0] == 'F' && p[1] == 'L' && p[2] == 'V', "Not a valid .flv file");
    FLVHeader *header = (FLVHeader *)p;
    if (header->flagAudio) {
        printf("has audio\n");
    }
    if (header->flagVideo) {
        printf("has video\n");
    }
    p += sizeof(FLVHeader); // skip Flv header
    p += preTagSizeLength;  // skip Previous Tag Size #0
    printf("%02x %02x %02x %02x\n", p[0], p[10], p[2], p[3]);
    ASSERT_LOG_RETURN(p[0] == TAG_AUDIO || p[0] == TAG_VIDEO || p[0] == TAG_SCRIPT, "invalid tag");

    while (p + sizeof(FlvTagHeader) < end) {
        FlvTagHeader *tag = (FlvTagHeader *)p;
        int length = tag->size[0] << 16 | tag->size[1] << 8 | tag->size[2];
        printf("Tag length: %d\n", length);
        p += sizeof(FlvTagHeader);

        if (p + length >= end) {
            printf("Incomplete .flv file, %p - %p\n", p, end);
            break;
        }

        if (tag->type == TAG_SCRIPT) {
            AMFDecoder decoder(p, length);
            std::vector<AMFValue> amfValues = decoder.GetValues();
            for (auto &item : amfValues) {
                std::cout << item.Dump() << std::endl;
            }

            if (!videoCallback && !audioCallback) {
                return;
            }
        } else if (tag->type == TAG_VIDEO) {
            if (!videoCallback) {
                continue;
            }

            AVCVideoTagHeader *tagHeader = (AVCVideoTagHeader *)p;
            assert(tagHeader->codec == CODEC_AVC);

            if (tagHeader->packetType == 0) {
                printf("video SPS/PPS frame\n");
                config.SetConfigurationPacket(tagHeader->data, length - sizeof(AVCVideoTagHeader));
                avcCNALULengthSize = config.GetNALULengthSize();
            } else if (tagHeader->packetType == 1) {
                const uint8_t *frameEnd = p + length;
                const uint8_t *naluStart = tagHeader->data;
                int naluSize = (naluStart[0] << 24) | (naluStart[1] << 16) | (naluStart[2] << 8) | naluStart[3];
                static const uint8_t startCode[4] = {0x00, 0x00, 0x00, 0x01};

                naluStart += avcCNALULengthSize; // skip nalu length field
                if (tagHeader->frameType == 1) {
                    printf("video key frame\n");
                    if ((*naluStart & 0x1f) == 0x05) {
                        std::string pps = config.GetPPS();
                        std::string sps = config.GetSPS();

                        assert(!pps.empty() && !sps.empty());
                        videoCallback(startCode, 4);
                        videoCallback((uint8_t *)sps.data(), sps.size());
                        videoCallback(startCode, 4);
                        videoCallback((uint8_t *)pps.data(), pps.size());

                        videoCallback(startCode + 1, 3);
                    } else {
                        videoCallback(startCode, 4);
                    }
                    videoCallback(naluStart, naluSize);
                } else if (tagHeader->frameType == 2) {
                    printf("video common frame\n");
                    videoCallback(startCode, 4);
                    videoCallback(naluStart, naluSize);
                }

                naluStart += naluSize;
                while (naluStart < frameEnd) {
                    // has other nalu
                    assert(naluStart + avcCNALULengthSize < frameEnd);
                    naluSize = (naluStart[0] << 24) | (naluStart[1] << 16) | (naluStart[2] << 8) | naluStart[3];
                    naluStart += avcCNALULengthSize;
                    assert(naluStart + naluSize <= frameEnd);
                    if ((*naluStart & 0x1f) == 0x05) {
                        std::string pps = config.GetPPS();
                        std::string sps = config.GetSPS();
                        assert(!pps.empty() && !sps.empty());

                        videoCallback(startCode, 4);
                        videoCallback((uint8_t *)sps.data(), sps.size());
                        videoCallback(startCode, 4);
                        videoCallback((uint8_t *)pps.data(), pps.size());

                        videoCallback(startCode + 1, 3);
                    } else {
                        videoCallback(startCode, 4);
                    }
                    videoCallback(naluStart, naluSize);
                    naluStart += naluSize;
                }
            }

        } else if (tag->type == TAG_AUDIO) {
            if (!audioCallback) {
                continue;
            }

            AACAudioTagHeader *tagHeader = (AACAudioTagHeader *)p;
            printf("audio codec: %d\n", tagHeader->codec);
            assert(tagHeader->codec == CODEC_AAC);
            printf("audio channel: %d, rate: %d, bit: %d, packetType: %d\n", tagHeader->channels, tagHeader->rate,
                   tagHeader->bits, tagHeader->packetType);

            int dataSize = length - sizeof(AACAudioTagHeader);
            if (tagHeader->packetType == AAC_HEADER) {
                AudioSpecificConfig config((char *)tagHeader->data, dataSize);
                printf("Audio specific config: %d-%d-%d\n", config.GetObjectType(), config.GetSampleRate(),
                       config.GetChannels());
                adtsHeader.SetChannel(config.GetChannels()).SetSamplingFrequency(config.GetSampleRate()).SetVBR();
            } else {
                adtsHeader.SetLength(dataSize + sizeof(ADTSHeader));
                audioCallback((uint8_t *)&adtsHeader, sizeof(ADTSHeader)); // aac header
                audioCallback(tagHeader->data, dataSize);                  // aac es data
            }
        }

        p += length;
        assert(p + preTagSizeLength <= end);

        int preTagSize = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
        assert(preTagSize == length + sizeof(FlvTagHeader));
        p += preTagSizeLength; // skip Previous Tag Size
    }

    if (p != end) {
        printf("Incomplete .flv file\n");
    }
}

int main(int argc, char *argv[]) {
    printf("flv-media\n");

    char operation = 0;
    char *infile = nullptr;
    if (!ProcessArgs(argc, argv, operation, infile)) {
        return 0;
    }

    if (operation == 'i') {
        printf("info %s\n", infile);
        auto reader = FileReader::Open(infile);
        if (reader == nullptr) {
            return 1;
        }
        ParseFlvFile(reader->data, reader->size, nullptr, nullptr);
    } else if (operation == 'm') {
        printf("mux %s\n", infile);
    } else if (operation == 'd') {
        printf("demux %s\n", infile);
        auto reader = FileReader::Open(infile);
        if (reader == nullptr) {
            return 1;
        }

        std::string name = std::string(infile);
        std::string prefix =
            name.substr(0, std::string(infile).find_last_of('.')) + '-' + std::to_string(time(nullptr));
        std::string videoName = prefix + ".h264";
        std::string audioName = prefix + ".aac";
        auto videoFile = FileWriter::Open(videoName);
        auto audioFile = FileWriter::Open(audioName);

        if (!videoFile || !audioFile) {
            return 1;
        }

        ParseFlvFile(
            reader->data, reader->size,
            [&](const uint8_t *data, size_t size) {
                printf("read video frame\n");
                videoFile->Write(data, size);
            },
            [&](const uint8_t *data, size_t size) {
                printf("read audio frame\n");
                audioFile->Write(data, size);
            });
    }

    printf("----\n");

    return 0;
}
