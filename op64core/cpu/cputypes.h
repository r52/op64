#pragma once

#include <cstdint>

#define PC_SIZE 4

// core types
enum CPUCoreTypes : uint8_t
{
    CPU_INTERPRETER,
    CPU_CACHED,
    CPU_JIT
};

enum RoundingMode_t : int32_t
{
    ROUND_MODE = 0x33F,
    FLOOR_MODE = 0x73F,
    CEIL_MODE = 0xB3F,
    TRUNC_MODE = 0xF3F
};

enum CP0Regs : uint8_t
{
    CP0_INDEX_REG = 0,
    CP0_RANDOM_REG,
    CP0_ENTRYLO0_REG,
    CP0_ENTRYLO1_REG,
    CP0_CONTEXT_REG,
    CP0_PAGEMASK_REG,
    CP0_WIRED_REG,
    /* 7 is unused */
    CP0_BADVADDR_REG = 8,
    CP0_COUNT_REG,
    CP0_ENTRYHI_REG,
    CP0_COMPARE_REG,
    CP0_STATUS_REG,
    CP0_CAUSE_REG,
    CP0_EPC_REG,
    CP0_PREVID_REG,
    CP0_CONFIG_REG,
    CP0_LLADDR_REG,
    CP0_WATCHLO_REG,
    CP0_WATCHHI_REG,
    CP0_XCONTEXT_REG,
    /* 21 - 27 are unused */
    CP0_TAGLO_REG = 28,
    CP0_TAGHI_REG,
    CP0_ERROREPC_REG,
    /* 31 is unused */
    CP0_REGS_COUNT = 32
};

/************************************************************************/
/* Instruction holder                                                   */
/************************************************************************/
union Instruction
{
    uint32_t code;

    struct
    {
        uint32_t offset : 16;
        uint32_t rt : 5;
        uint32_t rs : 5;
        uint32_t op : 6;
    };

    struct
    {
        uint32_t immediate : 16;
        uint32_t _dum2 : 5;
        uint32_t base : 5;
        uint32_t _dum1 : 6;
    };

    struct
    {
        uint32_t target : 26;
        uint32_t _dum3 : 6;
    };

    struct
    {
        uint32_t func : 6;
        uint32_t sa : 5;
        uint32_t rd : 5;
        uint32_t _dum4 : 16;
    };

    struct
    {
        uint32_t _dum6 : 6;
        uint32_t fd : 5;
        uint32_t fs : 5;
        uint32_t ft : 5;
        uint32_t fmt : 5;
        uint32_t _dum5 : 6;
    };

};

/************************************************************************/
/* Register struct                                                      */
/************************************************************************/

union Register64
{
    uint64_t u;
    int64_t s;
};

/************************************************************************/
/* Program Counter class                                                */
/************************************************************************/
class ProgramCounter
{
public:

    inline ProgramCounter(const ProgramCounter& x) = default;
    inline ProgramCounter(ProgramCounter&&) = default;
    inline ProgramCounter& operator=(ProgramCounter&&) = default;
    inline ProgramCounter& operator=(const ProgramCounter& x) = default;

    inline ProgramCounter(const uint32_t& x) :
        _addr(x)
    {}

    inline ProgramCounter& operator=(const uint32_t& x)
    {
        _addr = x;
        return *this;
    }

    inline ProgramCounter& operator=(uint32_t&& x)
    {
        _addr = std::move(x);
        return *this;
    }
    
    inline operator uint32_t()
    {
        return _addr;
    }

    inline ProgramCounter operator+(const uint32_t& x)
    {
        uint32_t result = _addr + (x * PC_SIZE);
        return ProgramCounter(result);
    }

    inline ProgramCounter& operator++()
    {
        _addr += PC_SIZE;
        return *this;
    }

    inline ProgramCounter operator++(int)
    {
        ProgramCounter tmp(*this);
        operator++();
        return tmp;
    }

    inline ProgramCounter& operator--()
    {
        _addr -= PC_SIZE;
        return *this;
    }

    inline ProgramCounter operator--(int)
    {
        ProgramCounter tmp(*this);
        operator--();
        return tmp;
    }

    inline ProgramCounter& operator+=(const uint32_t& x)
    {
        _addr += (x * PC_SIZE);
        return *this;
    }

    inline bool operator==(const ProgramCounter& x)
    {
        return (this->_addr == x._addr);
    }

private:
    uint_fast32_t _addr;
};
