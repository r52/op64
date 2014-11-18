#pragma once

#include <cstdint>

struct tlb_o {
    uint32_t page_mask[32];
    uint32_t vpn2[32];
    uint8_t global[32];
    uint8_t asid[32];
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
    static uint32_t virtual_to_physical_address(uint32_t address, TLBProbeMode mode);

    static void tlb_init(tlb_o& tlb);
    static int tlb_probe(const tlb_o& tlb, uint64_t vaddr, uint8_t vasid);
    static void tlb_read(const tlb_o& tlb, unsigned index, uint64_t *entry_hi);
    static void tlb_write(tlb_o& tlb, unsigned index, uint64_t entry_hi, uint64_t entry_lo_0, uint64_t entry_lo_1, uint32_t page_mask);
};
