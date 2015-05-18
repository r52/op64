#pragma once

#include <op64.h>

typedef struct {
    uint16_t Version;
    uint16_t Type;
    char Name[100];

    int32_t NormalMemory;
    int32_t MemoryBswaped;
} PLUGIN_INFO;

enum PLUGIN_TYPE {
    PLUGIN_TYPE_NONE = 0,
    PLUGIN_TYPE_RSP = 1,
    PLUGIN_TYPE_GFX = 2,
    PLUGIN_TYPE_AUDIO = 3,
    PLUGIN_TYPE_CONTROLLER = 4,
};

