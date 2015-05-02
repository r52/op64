/*
Copyright (c) 2014, Tyler J. Stachecki
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the Tyler J. Stachecki nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL TYLER J. STACHECKI BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <oppreproc.h>

#include "tlb.h"

#include <core/bus.h>
#include <rom/rom.h>
#include <cpu/icpu.h>
#include <cpu/cp0.h>

__align(static const int8_t one_hot_lut[256], CACHE_LINE_SIZE) = {
    -1,

    // 1
    0,

    // 2
    1, -1,

    // 4
    2, -1, -1, -1,

    // 8
    3, -1, -1, -1, -1, -1, -1, -1,

    // 16
    4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    // 32
    5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    // 64
    6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    // 128
    7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

uint32_t TLB::virtual_to_physical_address(uint32_t address, TLBProbeMode mode)
{
    if (address >= 0x7f000000 && address < 0x80000000 && (Bus::rom->getGameHacks() & GAME_HACK_GOLDENEYE))
    {
        //GoldenEye 007 hack allows for use of TLB.
        //Recoded by okaygo to support all US, J, and E ROMS.
        switch (Bus::rom->getHeader()->Country_code & 0xFF)
        {
        case 0x45:
            // U
            return 0xb0034b30 + (address & 0xFFFFFF);
            break;
        case 0x4A:
            // J
            return 0xb0034b70 + (address & 0xFFFFFF);
            break;
        case 0x50:
            // E
            return 0xb00329f0 + (address & 0xFFFFFF);
            break;
        default:
            // UNKNOWN COUNTRY CODE FOR GOLDENEYE USING AMERICAN VERSION HACK
            return 0xb0034b30 + (address & 0xFFFFFF);
            break;
        }
    }

    unsigned asid = Bus::cp0_reg[CP0_ENTRYHI_REG] & 0xFF;
    CP0* cp0 = Bus::cpu->getCp0();
    unsigned index;
    bool tlb_miss = tlb_probe(cp0->tlb, address, asid, &index);
    uint32_t page_mask = cp0->page_mask[index];
    unsigned select = ((page_mask + 1) & address) != 0;

    if (tlb_miss || !(cp0->state[index][select] & 2))
    {
        Bus::cpu->TLBRefillException(address, mode, tlb_miss);
        return 0x00000000;
    }

    return (0x80000000 + ((cp0->pfn[index][select]) | (address & page_mask)));
}

/* Ported cen64 tlb implementation */
void TLB::tlb_init(tlb_o& tlb)
{
    for (uint32_t i = 0; i < 32; i++)
    {
        tlb.vpn2.data[i] = ~0;
    }
}

void TLB::tlb_write(tlb_o& tlb, unsigned index, uint64_t entry_hi, uint64_t entry_lo_0, uint64_t entry_lo_1, uint32_t page_mask)
{
    tlb.page_mask.data[index] = ~(page_mask >> 13);

    tlb.vpn2.data[index] =
        (entry_hi >> 35 & 0x18000000U) |
        (entry_hi >> 13 & 0x7FFFFFF);

    tlb.global[index] = (entry_lo_0 & 0x1) && (entry_lo_1 & 0x1) ? 0xFF : 0x00;
    tlb.asid[index] = entry_hi & 0xFF;
}

void TLB::tlb_read(const tlb_o& tlb, unsigned index, uint64_t *entry_hi)
{
    *entry_hi =
        ((tlb.vpn2.data[index] & 0x18000000LLU) << 35) |
        ((tlb.vpn2.data[index] & 0x7FFFFFFLLU) << 13) |
        ((tlb.global[index] & 1) << 12) |
        (tlb.asid[index]);
}

bool TLB::tlb_probe(const tlb_o& tlb, uint64_t vaddr, uint8_t vasid, unsigned* index)
{
    int one_hot_idx;

    uint32_t vpn2 =
        (vaddr >> 35 & 0x18000000U) |
        (vaddr >> 13 & 0x7FFFFFF);

    __m128i vpn = _mm_set1_epi32(vpn2);
    __m128i asid = _mm_set1_epi8(vasid);

    // Scan 8 entries in parallel.
    // op64: the original loop from cen64 has a bug where i += 8
    // which terminates the loop after just 1 iteration (since 32/8 = 4)
    for (unsigned i = 0; i < 32 / 8; i++)
    {
        unsigned j = i * 8;
        __m128i check_l, check_h, vpn_check;
        __m128i check_a, check_g, asid_check;
        __m128i check;

        __m128i page_mask_l = _mm_load_si128((__m128i*) (tlb.page_mask.data + j + 0));
        __m128i page_mask_h = _mm_load_si128((__m128i*) (tlb.page_mask.data + j + 4));
        __m128i vpn_l = _mm_load_si128((__m128i*) (tlb.vpn2.data + j + 0));
        __m128i vpn_h = _mm_load_si128((__m128i*) (tlb.vpn2.data + j + 4));

        // Check for matching VPNs.
        check_l = _mm_and_si128(vpn, page_mask_l);
        check_l = _mm_cmpeq_epi32(check_l, vpn_l);
        check_h = _mm_and_si128(vpn, page_mask_h);
        check_h = _mm_cmpeq_epi32(check_h, vpn_h);
        vpn_check = _mm_packs_epi32(check_l, check_h);
        vpn_check = _mm_packs_epi16(vpn_check, vpn_check);

        // Check for matching ASID/global, too.
        check_g = _mm_loadl_epi64((__m128i*) (tlb.global + j));
        check_a = _mm_loadl_epi64((__m128i*) (tlb.asid + j));
        asid_check = _mm_cmpeq_epi8(check_a, asid);
        asid_check = _mm_or_si128(check_g, asid_check);

        // Match only on VPN match && (asid match || global)
        check = _mm_and_si128(vpn_check, asid_check);
        if ((one_hot_idx = _mm_movemask_epi8(check)) != 0)
        {
            *index = j + one_hot_lut[one_hot_idx & 0xFF];
            return false;
        }
    }

    *index = 0;
    return true;
}
