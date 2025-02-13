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
