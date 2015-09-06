#pragma once

#include <tlb/tlb.h>
#include <core/bus.h>
#include <rcp/rcp.h>
#include <rom/rom.h>


enum DataSize
{
    SIZE_WORD = 0,
    SIZE_DWORD = 1,
    SIZE_HWORD = 2,
    SIZE_BYTE = 3,
    NUM_DATA_SIZES
};

typedef struct
{
    uint32_t addr;
    uint32_t size;
    uint32_t width;
    uint32_t height;
} FrameBufferInfo;

class IMemory
{

    typedef void(IMemory::*dataptr_read)(RCPInterface&, uint32_t&, uint64_t*);
    typedef void(IMemory::*dataptr_write)(RCPInterface&, uint32_t, uint64_t);

public:
    virtual ~IMemory();

    virtual bool initialize(Bus* bus);
    virtual void uninitialize(Bus* bus);
    virtual void readmem(uint32_t& address, uint64_t* dest, DataSize size) = 0;
    virtual void writemem(uint32_t address, uint64_t src, DataSize size) = 0;

    // shouldn't be using these except with mupen interpreter
    virtual void unprotectFramebuffer(void) = 0;
    virtual void protectFramebuffer(void) = 0;

    inline uint32_t* fastFetch(uint32_t address)
    {
        if ((address & 0xc0000000) != 0x80000000)
        {
            if (!(address = TLB::virtual_to_physical_address(_bus, address, TLB_FAST_READ)))
            {
                return nops;
            }
        }

        address &= 0x1ffffffc;

        if (address < RDRAM_SIZE)
        {
            return (uint32_t*)((uint8_t*)Bus::rdram.mem + address);
        }
        else if (address >= 0x10000000)
        {
            return (uint32_t*)((uint8_t*)_bus->rom->getImage() + address - 0x10000000);
        }
        else if ((address & 0xffffe000) == 0x04000000)
        {
            return (uint32_t*)((uint8_t*)Bus::rcp.sp.mem + (address & 0x1ffc));
        }

        return nullptr;
    }

protected:
    // Data read
    void read_size_byte(RCPInterface& device, uint32_t& address, uint64_t* dest);
    void read_size_half(RCPInterface& device, uint32_t& address, uint64_t* dest);
    void read_size_word(RCPInterface& device, uint32_t& address, uint64_t* dest);
    void read_size_dword(RCPInterface& device, uint32_t& address, uint64_t* dest);

    // Data write
    void write_size_byte(RCPInterface& device, uint32_t address, uint64_t src);
    void write_size_half(RCPInterface& device, uint32_t address, uint64_t src);
    void write_size_word(RCPInterface& device, uint32_t address, uint64_t src);
    void write_size_dword(RCPInterface& device, uint32_t address, uint64_t src);

    // Read functions
    void read_nothing(uint32_t& address, uint64_t* dest, DataSize size);
    void read_nomem(uint32_t& address, uint64_t* dest, DataSize size);

    void read_rdram(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rdram_reg(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rsp_mem(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rsp_reg(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rsp_stat(uint32_t& address, uint64_t* dest, DataSize size);
    void read_dp(uint32_t& address, uint64_t* dest, DataSize size);
    void read_dps(uint32_t& address, uint64_t* dest, DataSize size);
    void read_mi(uint32_t& address, uint64_t* dest, DataSize size);
    void read_vi(uint32_t& address, uint64_t* dest, DataSize size);
    void read_ai(uint32_t& address, uint64_t* dest, DataSize size);
    void read_pi(uint32_t& address, uint64_t* dest, DataSize size);
    void read_ri(uint32_t& address, uint64_t* dest, DataSize size);
    void read_si(uint32_t& address, uint64_t* dest, DataSize size);

    void read_flashram_status(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rom(uint32_t& address, uint64_t* dest, DataSize size);
    void read_pif(uint32_t& address, uint64_t* dest, DataSize size);

    void read_rdramFB(uint32_t& address, uint64_t* dest, DataSize size);

    // Write functions
    void write_nothing(uint32_t address, uint64_t src, DataSize size);
    void write_nomem(uint32_t address, uint64_t src, DataSize size);

    void write_rdram(uint32_t address, uint64_t src, DataSize size);
    void write_rdram_reg(uint32_t address, uint64_t src, DataSize size);
    void write_rsp_mem(uint32_t address, uint64_t src, DataSize size);
    void write_rsp_reg(uint32_t address, uint64_t src, DataSize size);
    void write_rsp_stat(uint32_t address, uint64_t src, DataSize size);
    void write_dp(uint32_t address, uint64_t src, DataSize size);
    void write_dps(uint32_t address, uint64_t src, DataSize size);
    void write_mi(uint32_t address, uint64_t src, DataSize size);
    void write_vi(uint32_t address, uint64_t src, DataSize size);
    void write_ai(uint32_t address, uint64_t src, DataSize size);
    void write_pi(uint32_t address, uint64_t src, DataSize size);
    void write_ri(uint32_t address, uint64_t src, DataSize size);
    void write_si(uint32_t address, uint64_t src, DataSize size);

    void write_flashram_dummy(uint32_t address, uint64_t src, DataSize size);
    void write_flashram_command(uint32_t address, uint64_t src, DataSize size);
    void write_rom(uint32_t address, uint64_t src, DataSize size);
    void write_pif(uint32_t address, uint64_t src, DataSize size);

    void write_rdramFB(uint32_t address, uint64_t src, DataSize size);

protected:
    // not owned
    Bus* _bus = nullptr;

    // i/o size jump table
    dataptr_read readsize[NUM_DATA_SIZES] = { &IMemory::read_size_word, &IMemory::read_size_dword, &IMemory::read_size_half, &IMemory::read_size_byte };
    dataptr_write writesize[NUM_DATA_SIZES] = { &IMemory::write_size_word, &IMemory::write_size_dword, &IMemory::write_size_half, &IMemory::write_size_byte };

    // framebuffer store
    FrameBufferInfo fbInfo[6];
    uint8_t framebufferRead[0x800];

private:
	uint32_t nops[2] = { 0 };

};
