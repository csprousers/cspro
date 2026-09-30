#include <StandardIncludes/cspro_no_mfc.h>
#include "PlatformInterface.h"


PlatformInterface::PlatformInterface()
    :   m_pApplicationInterface(nullptr)
{
}

PlatformInterface* PlatformInterface::GetInstance()
{
    static PlatformInterface instance;
    return &instance;
}
