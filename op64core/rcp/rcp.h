#pragma once

#include "rdramcontroller.h"
#include "audiointerface.h"
#include "dpcinterface.h"
#include "dpsinterface.h"
#include "mipsinterface.h"
#include "peripheralinterface.h"
#include "rdraminterface.h"
#include "serialinterface.h"
#include "rspinterface.h"
#include "videointerface.h"

// Container for RCP interfaces
class RCP
{
public:
    AudioInterface ai;
    DPCInterface dpc;
    DPSInterface dps;
    MIPSInterface mi;
    PeripheralInterface pi;
    RDRAMInterface ri;
    SerialInterface si;
    RSPInterface sp;
    VideoInterface vi;
};
