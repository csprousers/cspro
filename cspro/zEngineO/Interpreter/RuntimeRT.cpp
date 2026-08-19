#include "stdafx.h"
#include "IncludesRT.h"


EngineAppType LogicInterpreter::GetEngineAppType() const noexcept
{
    return ( m_engineData->application != nullptr ) ? m_engineData->application->GetEngineAppType() :
                                                      EngineAppType::Invalid;
}
