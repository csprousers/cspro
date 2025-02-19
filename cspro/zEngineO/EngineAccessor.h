#pragma once

namespace Listing { class Lister; }
class Symbol;
class SystemMessageIssuer;


class EngineAccessor
{
public:
    virtual ~EngineAccessor() { }

    virtual void ea_SetVarTValueSetter(std::function<void(Symbol* pVarT, std::wstring value)> setter) = 0;
    virtual void ea_SetVarTValue(Symbol* pVarT, std::wstring value) = 0;

    virtual SystemMessageIssuer& ea_GetSystemMessageIssuer() = 0;

    virtual std::set<int>& ea_GetPersistentSymbolsNeedingResetSet() = 0;

    virtual Listing::Lister* ea_GetLister() = 0;
};
