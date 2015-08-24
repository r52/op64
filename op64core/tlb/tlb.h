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
EVEN IF AD
*/

#pragma once

#include <cstdint>
#include <emmintrin.h>

union aligned_tlb_data {
    __m128i __align[8];
    uint32_t data[32];
};

struct tlb_o {
    union aligned_tlb_data page_mask;
    union aligned_tlb_data vpn2;
    uint8_t global[32];
    uint8_t asid[32];
};

enum TLBProbeMode : uint8_t
{
    TLB_READ = 0,
    TLB_WRITE,
    TLB_FAST_READ
};

class Bus;

class TLB
{
public:
    static uint32_t virtual_to_physical_address(Bus* bus, uint32_t address, TLBProbeMode mode);

    static void tlb_init(tlb_o& tlb);

    // Return true if miss, false otherwise
    static bool tlb_probe(const tlb_o& tlb, uint64_t vaddr, uint8_t vasid, unsigned* index);
    static void tlb_read(const tlb_o& tlb, unsigned index, uint64_t *entry_hi);
    static void tlb_write(tlb_o& tlb, unsigned index, uint64_t entry_hi, uint64_t entry_lo_0, uint64_t entry_lo_1, uint32_t page_mask);
};
