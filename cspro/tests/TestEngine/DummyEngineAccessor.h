#pragma once

#include <zEngineO/EngineAccessor.h>

#pragma warning(push)
#pragma warning(disable:4100)

#define NOT_IMPLEMENTED(x) { throw CSProException("Not implemented: %s", #x); }


class DummyEngineAccessor : public EngineAccessor
{
public:
    void ea_SetVarTValueSetter(std::function<void(Symbol* pVarT, std::wstring)> setter) override { NOT_IMPLEMENTED(ea_SetVarTValueSetter) }
    void ea_SetVarTValue(Symbol* symbol, std::wstring value) override { NOT_IMPLEMENTED(ea_SetVarTValue) }
    std::shared_ptr<SystemMessageIssuer> ea_GetSharedSystemMessageIssuer() override { NOT_IMPLEMENTED(ea_GetSharedSystemMessageIssuer) }
    std::set<int>& ea_GetPersistentSymbolsNeedingResetSet() override { NOT_IMPLEMENTED(ea_GetPersistentSymbolsNeedingResetSet) }
    Listing::Lister* ea_GetLister() override { NOT_IMPLEMENTED(ea_GetLister) }
};

#pragma warning(pop)
