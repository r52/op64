#pragma once

#define PLUGIN_NONE                 1
#define PLUGIN_MEMPAK               2
#define PLUGIN_RUMBLE_PAK           3 /* not implemented for non raw data */
#define PLUGIN_TRANSFER_PAK         4 /* not implemented for non raw data */
#define PLUGIN_RAW                  5 /* the controller plugin is passed in raw data */

typedef struct _controller_data {
    int Present;
    int RawData;
    int  Plugin;
} CONTROL;

typedef union {
    unsigned int Value;
    struct {
        unsigned R_DPAD : 1;
        unsigned L_DPAD : 1;
        unsigned D_DPAD : 1;
        unsigned U_DPAD : 1;
        unsigned START_BUTTON : 1;
        unsigned Z_TRIG : 1;
        unsigned B_BUTTON : 1;
        unsigned A_BUTTON : 1;

        unsigned R_CBUTTON : 1;
        unsigned L_CBUTTON : 1;
        unsigned D_CBUTTON : 1;
        unsigned U_CBUTTON : 1;
        unsigned R_TRIG : 1;
        unsigned L_TRIG : 1;
        unsigned Reserved1 : 1;
        unsigned Reserved2 : 1;

        signed   X_AXIS : 8;
        signed   Y_AXIS : 8;
    };
} BUTTONS;

#ifdef _MSC_VER
#include <windows.h>
typedef struct {
    HWND hMainWindow;
    HINSTANCE hinst;

    BOOL MemoryBswaped;		// If this is set to TRUE, then the memory has been pre
    //   bswap on a dword (32 bits) boundry, only effects header. 
    //	eg. the first 8 bytes are stored like this:
    //        4 3 2 1   8 7 6 5
    BYTE * HEADER;			// This is the rom header (first 40h bytes of the rom)
    CONTROL *Controls;		// A pointer to an array of 4 controllers .. eg:
    // CONTROL Controls[4];
} CONTROL_INFO;
#else
typedef struct {
    CONTROL *Controls;      /* A pointer to an array of 4 controllers .. eg:
                            CONTROL Controls[4]; */
} CONTROL_INFO;
#endif
