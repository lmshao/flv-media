//
// Copyright (c) 2023 SHAO Liming<lmshao@163.com>. All rights reserved.
//

#include "AMF.h"
#include "AudioTag.h"
#include "FLV.h"
#include "VideoTag.h"
#include <cstdio>
#include <getopt.h>

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

int main(int argc, char *argv[]) {
    printf("flv-media\n");

    char operation = 0;
    char *infile = nullptr;
    if (!ProcessArgs(argc, argv, operation, infile)) {
        return 0;
    }

    if (operation == 'i') {
        printf("info %s\n", infile);
    } else if (operation == 'm') {
        printf("mux %s\n", infile);
    } else if (operation == 'd') {
        printf("demux %s\n", infile);
    }

    return 0;
}
