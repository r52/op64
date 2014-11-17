#pragma once

#include <cstdint>

struct tlb_entry
{
    int16_t mask;
    int32_t vpn2;
    int8_t g;
    uint8_t asid;
    int32_t pfn_even;
    int8_t c_even;
    int8_t d_even;
    int8_t v_even;
    int32_t pfn_odd;
    int8_t c_odd;
    int8_t d_odd;
    int8_t v_odd;
    int8_t r;
    //int check_parity_mask;

    uint32_t start_even;
    uint32_t end_even;
    uint32_t phys_even;
    uint32_t start_odd;
    uint32_t end_odd;
    uint32_t phys_odd;
};

enum TLBProbeMode
{
    TLB_READ = 0,
    TLB_WRITE,
    TLB_FAST_READ
};

class TLB
{
public:
    static void tlb_unmap(tlb_entry *entry);
    static void tlb_map(tlb_entry *entry);
    static uint32_t virtual_to_physical_address(uint32_t addresse, TLBProbeMode mode);

    static tlb_entry tlb_entry_table[32];
    static uint32_t tlb_lookup_read[0x100000];
    static uint32_t tlb_lookup_write[0x100000];
};
