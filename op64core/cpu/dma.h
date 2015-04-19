#pragma once

class DMA
{
public:
    static void readSP(void);
    static void writeSP(void);

    static void readPI(void);
    static void writePI(void);

    static void readSI(void);
    static void writeSI(void);
};
