#include "tlb.h"
#include "bus.h"
#include "rom.h"
#include "icpu.h"
#include "compiler.h"


tlb_entry TLB::tlb_entry_table[32];
uint32_t TLB::tlb_lookup_read[0x100000];
uint32_t TLB::tlb_lookup_write[0x100000];

uint32_t TLB::virtual_to_physical_address(uint32_t addresse, int32_t w)
{
    if (addresse >= 0x7f000000 && addresse < 0x80000000 && false /*isGoldeneyeRom*/)
    {
        /**************************************************
        GoldenEye 007 hack allows for use of TLB.
        Recoded by okaygo to support all US, J, and E ROMS.
        **************************************************/
        switch (Bus::rom->getHeader()->Country_code & 0xFF)
        {
        case 0x45:
            // U
            return 0xb0034b30 + (addresse & 0xFFFFFF);
            break;
        case 0x4A:
            // J
            return 0xb0034b70 + (addresse & 0xFFFFFF);
            break;
        case 0x50:
            // E
            return 0xb00329f0 + (addresse & 0xFFFFFF);
            break;
        default:
            // UNKNOWN COUNTRY CODE FOR GOLDENEYE USING AMERICAN VERSION HACK
            return 0xb0034b30 + (addresse & 0xFFFFFF);
            break;
        }
    }
    if (w == 1)
    {
        if (tlb_lookup_write[addresse >> 12])
            return (tlb_lookup_write[addresse >> 12] & 0xFFFFF000) | (addresse & 0xFFF);
    }
    else
    {
        if (tlb_lookup_read[addresse >> 12])
            return (tlb_lookup_read[addresse >> 12] & 0xFFFFF000) | (addresse & 0xFFF);
    }
    //printf("tlb exception !!! @ %x, %x, add:%x\n", addresse, w, PC->addr);
    //getchar();
    Bus::cpu->TLB_refill_exception(addresse, w);
    //return 0x80000000;
    return 0x00000000;
}

void TLB::tlb_unmap(tlb_entry *entry)
{
    unsigned int i;

    if (entry->v_even)
    {
        vec_for (i = entry->start_even; i < entry->end_even; i += 0x1000)
        {
            tlb_lookup_read[i >> 12] = 0;
        }

        if (entry->d_even)
        {
            vec_for (i = entry->start_even; i < entry->end_even; i += 0x1000)
            {
                tlb_lookup_write[i >> 12] = 0;
            }
        }
    }

    if (entry->v_odd)
    {
        vec_for (i = entry->start_odd; i < entry->end_odd; i += 0x1000)
        {
            tlb_lookup_read[i >> 12] = 0;
        }

        if (entry->d_odd)
        {
            vec_for (i = entry->start_odd; i < entry->end_odd; i += 0x1000)
            {
                tlb_lookup_write[i >> 12] = 0;
            }
        }
    }
}

void TLB::tlb_map(tlb_entry *entry)
{
    unsigned int i;

    if (entry->v_even)
    {
        if (entry->start_even < entry->end_even &&
            !(entry->start_even >= 0x80000000 && entry->end_even < 0xC0000000) &&
            entry->phys_even < 0x20000000)
        {
            vec_for (i = entry->start_even; i < entry->end_even; i += 0x1000)
            {
                tlb_lookup_read[i >> 12] = 0x80000000 | (entry->phys_even + (i - entry->start_even) + 0xFFF);
            }

            if (entry->d_even)
            {
                vec_for (i = entry->start_even; i < entry->end_even; i += 0x1000)
                {
                    tlb_lookup_write[i >> 12] = 0x80000000 | (entry->phys_even + (i - entry->start_even) + 0xFFF);
                }
            }
        }
    }

    if (entry->v_odd)
    {
        if (entry->start_odd < entry->end_odd &&
            !(entry->start_odd >= 0x80000000 && entry->end_odd < 0xC0000000) &&
            entry->phys_odd < 0x20000000)
        {
            vec_for (i = entry->start_odd; i < entry->end_odd; i += 0x1000)
            {
                tlb_lookup_read[i >> 12] = 0x80000000 | (entry->phys_odd + (i - entry->start_odd) + 0xFFF);
            }

            if (entry->d_odd)
            {
                vec_for (i = entry->start_odd; i < entry->end_odd; i += 0x1000)
                {
                    tlb_lookup_write[i >> 12] = 0x80000000 | (entry->phys_odd + (i - entry->start_odd) + 0xFFF);
                }
            }
        }
    }
}
