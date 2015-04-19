#pragma once
#include <cstdint>
#include <deque>

#define VI_INT      0x001
#define COMPARE_INT 0x002
#define CHECK_INT   0x004
#define SI_INT      0x008
#define PI_INT      0x010
#define SPECIAL_INT 0x020
#define AI_INT      0x040
#define SP_INT      0x080
#define DP_INT      0x100
#define HW2_INT     0x200
#define NMI_INT     0x400

struct Interrupt
{
    int32_t type;
    uint32_t count;

    Interrupt(int32_t t) : type(t), count(0)
    {}

    Interrupt(int32_t t, uint32_t c) : type(t), count(c)
    {}

    Interrupt(const Interrupt& x) : type(x.type), count(x.count)
    {}

    Interrupt& operator= (const Interrupt& x)
    {
        if (this != &x)
        {
            type = x.type;
            count = x.count;
        }

        return *this;
    }

    bool operator<(const Interrupt& i) const;
    bool operator==(const Interrupt& i) const;
};


typedef std::deque<Interrupt> InterruptQueue;

class InterruptHandler
{
public:
    InterruptHandler(void);
    ~InterruptHandler(void);

    void initialize(void);
    void addInterruptEvent(int32_t type, uint32_t delay);
    void addInterruptEventCount(int32_t type, uint32_t count);
    void generateInterrupt(void);
    void checkInterrupt(void);
    uint32_t findEvent(int32_t type);
    void deleteEvent(int32_t type);
    void translateEventQueueBy(uint32_t base);

    void softReset(void);

private:
    void doHardReset(void);
    void popInterruptEvent(void);

private:
    InterruptQueue q;

    int32_t _vi_counter = 0;
};
