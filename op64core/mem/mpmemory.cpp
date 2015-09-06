#include <algorithm>

#include <oplog.h>
#include <oputil.h>

#include "mpmemory.h"

#include <plugin/plugincontainer.h>
#include <plugin/gfxplugin.h>


bool MPMemory::initialize(Bus* bus)
{
    LOG_INFO(MPMemory) << "Initializing...";

    if (!IMemory::initialize(bus))
    {
        return false;
    }

    fill_array(readmem_table, 0, 0x10000, &MPMemory::read_nomem);
    fill_array(writemem_table, 0, 0x10000, &MPMemory::write_nomem);

    fill_array(readmem_table, 0x8000, 0x80, &MPMemory::read_rdram);
    fill_array(readmem_table, 0xa000, 0x80, &MPMemory::read_rdram);
    fill_array(writemem_table, 0x8000, 0x80, &MPMemory::write_rdram);
    fill_array(writemem_table, 0xa000, 0x80, &MPMemory::write_rdram);

    fill_array(readmem_table, 0x8080, 0x370, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa080, 0x370, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8080, 0x370, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa080, 0x370, &MPMemory::write_nothing);

    // rdram reg
    readmem_table[0x83f0] = &MPMemory::read_rdram_reg;
    readmem_table[0xa3f0] = &MPMemory::read_rdram_reg;
    writemem_table[0x83f0] = &MPMemory::write_rdram_reg;
    writemem_table[0xa3f0] = &MPMemory::write_rdram_reg;

    fill_array(readmem_table, 0x83f1, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa3f1, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x83f1, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa3f1, 0xf, &MPMemory::write_nothing);


    // sp mem
    readmem_table[0x8400] = &MPMemory::read_rsp_mem;
    readmem_table[0xa400] = &MPMemory::read_rsp_mem;
    writemem_table[0x8400] = &MPMemory::write_rsp_mem;
    writemem_table[0xa400] = &MPMemory::write_rsp_mem;

    fill_array(readmem_table, 0x8401, 0x3, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa401, 0x3, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8401, 0x3, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa401, 0x3, &MPMemory::write_nothing);

    //sp reg
    readmem_table[0x8404] = &MPMemory::read_rsp_reg;
    readmem_table[0xa404] = &MPMemory::read_rsp_reg;
    writemem_table[0x8404] = &MPMemory::write_rsp_reg;
    writemem_table[0xa404] = &MPMemory::write_rsp_reg;

    fill_array(readmem_table, 0x8405, 0x3, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa405, 0x3, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8405, 0x3, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa405, 0x3, &MPMemory::write_nothing);

    readmem_table[0x8408] = &MPMemory::read_rsp_stat;
    readmem_table[0xa408] = &MPMemory::read_rsp_stat;
    writemem_table[0x8408] = &MPMemory::write_rsp_stat;
    writemem_table[0xa408] = &MPMemory::write_rsp_stat;

    fill_array(readmem_table, 0x8409, 0x7, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa409, 0x7, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8409, 0x7, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa409, 0x7, &MPMemory::write_nothing);

    // dp reg
    readmem_table[0x8410] = &MPMemory::read_dp;
    readmem_table[0xa410] = &MPMemory::read_dp;
    writemem_table[0x8410] = &MPMemory::write_dp;
    writemem_table[0xa410] = &MPMemory::write_dp;

    fill_array(readmem_table, 0x8411, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa411, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8411, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa411, 0xf, &MPMemory::write_nothing);

    // dps reg
    readmem_table[0x8420] = &MPMemory::read_dps;
    readmem_table[0xa420] = &MPMemory::read_dps;
    writemem_table[0x8420] = &MPMemory::write_dps;
    writemem_table[0xa420] = &MPMemory::write_dps;

    fill_array(readmem_table, 0x8421, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa421, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8421, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa421, 0xf, &MPMemory::write_nothing);


    // mi reg
    readmem_table[0xa830] = &MPMemory::read_mi;
    readmem_table[0xa430] = &MPMemory::read_mi;
    writemem_table[0xa830] = &MPMemory::write_mi;
    writemem_table[0xa430] = &MPMemory::write_mi;

    fill_array(readmem_table, 0x8431, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa431, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8431, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa431, 0xf, &MPMemory::write_nothing);

    // vi reg
    readmem_table[0x8440] = &MPMemory::read_vi;
    readmem_table[0xa440] = &MPMemory::read_vi;
    writemem_table[0x8440] = &MPMemory::write_vi;
    writemem_table[0xa440] = &MPMemory::write_vi;

    fill_array(readmem_table, 0x8441, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa441, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8441, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa441, 0xf, &MPMemory::write_nothing);


    // ai reg
    readmem_table[0x8450] = &MPMemory::read_ai;
    readmem_table[0xa450] = &MPMemory::read_ai;
    writemem_table[0x8450] = &MPMemory::write_ai;
    writemem_table[0xa450] = &MPMemory::write_ai;

    fill_array(readmem_table, 0x8451, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa451, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8451, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa451, 0xf, &MPMemory::write_nothing);

    // pi reg
    readmem_table[0x8460] = &MPMemory::read_pi;
    readmem_table[0xa460] = &MPMemory::read_pi;
    writemem_table[0x8460] = &MPMemory::write_pi;
    writemem_table[0xa460] = &MPMemory::write_pi;

    fill_array(readmem_table, 0x8461, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa461, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8461, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa461, 0xf, &MPMemory::write_nothing);

    // ri reg
    readmem_table[0x8470] = &MPMemory::read_ri;
    readmem_table[0xa470] = &MPMemory::read_ri;
    writemem_table[0x8470] = &MPMemory::write_ri;
    writemem_table[0xa470] = &MPMemory::write_ri;

    fill_array(readmem_table, 0x8471, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa471, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8471, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa471, 0xf, &MPMemory::write_nothing);

    // si reg
    readmem_table[0x8480] = &MPMemory::read_si;
    readmem_table[0xa480] = &MPMemory::read_si;
    writemem_table[0x8480] = &MPMemory::write_si;
    writemem_table[0xa480] = &MPMemory::write_si;

    fill_array(readmem_table, 0x8481, 0x37f, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa481, 0x37f, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8481, 0x37f, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa481, 0x37f, &MPMemory::write_nothing);

    // flashram
    readmem_table[0x8800] = &MPMemory::read_flashram_status;
    readmem_table[0xa800] = &MPMemory::read_flashram_status;
    readmem_table[0x8801] = &MPMemory::read_nothing;
    readmem_table[0xa801] = &MPMemory::read_nothing;
    writemem_table[0x8800] = &MPMemory::write_flashram_dummy;
    writemem_table[0xa800] = &MPMemory::write_flashram_dummy;
    writemem_table[0x8801] = &MPMemory::write_flashram_command;
    writemem_table[0xa801] = &MPMemory::write_flashram_command;

    fill_array(readmem_table, 0x8802, 0x7fe, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa802, 0x7fe, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8802, 0x7fe, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa802, 0x7fe, &MPMemory::write_nothing);

    // rom
    uint32_t rom_size = bus->rom->getSize();
    fill_array(readmem_table, 0x9000, (rom_size >> 16), &MPMemory::read_rom);
    fill_array(readmem_table, 0xb000, (rom_size >> 16), &MPMemory::read_rom);
    fill_array(writemem_table, 0x9000, (rom_size >> 16), &MPMemory::write_nothing);
    fill_array(writemem_table, 0xb000, (rom_size >> 16), &MPMemory::write_rom);

    fill_array(readmem_table, 0x9000 + (rom_size >> 16), 0x9fc0 - (0x9000 + (rom_size >> 16)), &MPMemory::read_nothing);
    fill_array(readmem_table, 0xb000 + (rom_size >> 16), 0xbfc0 - (0xb000 + (rom_size >> 16)), &MPMemory::read_nothing);
    fill_array(writemem_table, 0x9000 + (rom_size >> 16), 0x9fc0 - (0x9000 + (rom_size >> 16)), &MPMemory::write_nothing);
    fill_array(writemem_table, 0xb000 + (rom_size >> 16), 0xbfc0 - (0xb000 + (rom_size >> 16)), &MPMemory::write_nothing);


    // pif
    readmem_table[0x9fc0] = &MPMemory::read_pif;
    readmem_table[0xbfc0] = &MPMemory::read_pif;
    writemem_table[0x9fc0] = &MPMemory::write_pif;
    writemem_table[0xbfc0] = &MPMemory::write_pif;

    fill_array(readmem_table, 0x9fc1, 0x3f, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xbfc1, 0x3f, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x9fc1, 0x3f, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xbfc1, 0x3f, &MPMemory::write_nothing);

    return true;
}

void MPMemory::unprotectFramebuffer(void)
{
    if (_bus->plugins->gfx()->fbGetInfo && _bus->plugins->gfx()->fbRead && _bus->plugins->gfx()->fbWrite &&
        fbInfo[0].addr)
    {
        for (uint32_t i = 0; i < 6; i++)
        {
            if (fbInfo[i].addr)
            {
                uint32_t start = fbInfo[i].addr & 0x7FFFFF;
                uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
                start = start >> 16;
                end = end >> 16;

                fill_array(readmem_table, 0x8000 + start, end - start + 1, &MPMemory::read_rdram);
                fill_array(readmem_table, 0xa000 + start, end - start + 1, &MPMemory::read_rdram);
                fill_array(writemem_table, 0x8000 + start, end - start + 1, &MPMemory::write_rdram);
                fill_array(writemem_table, 0xa000 + start, end - start + 1, &MPMemory::write_rdram);
            }
        }
    }
}

void MPMemory::protectFramebuffer(void)
{
    if (_bus->plugins->gfx()->fbGetInfo && _bus->plugins->gfx()->fbRead && _bus->plugins->gfx()->fbWrite)
    {
        _bus->plugins->gfx()->fbGetInfo(fbInfo);
    }

    if (_bus->plugins->gfx()->fbGetInfo && _bus->plugins->gfx()->fbRead && _bus->plugins->gfx()->fbWrite
        && fbInfo[0].addr)
    {
        for (uint32_t i = 0; i < 6; i++)
        {
            if (fbInfo[i].addr)
            {
                uint32_t j;
                uint32_t start = fbInfo[i].addr & 0x7FFFFF;
                uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
                uint32_t start1 = start;
                uint32_t end1 = end;

                start >>= 16;
                end >>= 16;

                fill_array(readmem_table, 0x8000 + start, end - start + 1, &MPMemory::read_rdramFB);
                fill_array(readmem_table, 0xa000 + start, end - start + 1, &MPMemory::read_rdramFB);
                fill_array(writemem_table, 0x8000 + start, end - start + 1, &MPMemory::write_rdramFB);
                fill_array(writemem_table, 0xa000 + start, end - start + 1, &MPMemory::write_rdramFB);

                start <<= 4;
                end <<= 4;
                for (j = start; j <= end; j++)
                {
                    if (j >= start1 && j <= end1)
                    {
                        framebufferRead[j] = 1;
                    }
                    else
                    {
                        framebufferRead[j] = 0;
                    }
                }
            }
        }
    }
}
