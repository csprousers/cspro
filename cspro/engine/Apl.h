#pragma once

#include <zLogicO/Symbol.h>
#include <engine/Defines.h>
#include <zEngineO/AllSymbolDeclarations.h>

namespace Logic { class SourceBuffer; }


class CSymbolApl : public Symbol
{
public:
    CSymbolApl()
        :   Symbol("GLOBAL", SymbolType::Application),
            ApplicationType(ModuleType::None),
            MaxLevel(0)
    {
    }

    void AddFlow(FLOW* flow)               { m_flows_pre80.push_back(flow); }
    int GetNumFlows() const                { return static_cast<int>(m_flows_pre80.size()); }
    FLOW* GetFlowAt(int flow_number) const { return m_flows_pre80[flow_number]; }

public:
    ModuleType ApplicationType;     // Application Type
    std::string ApplicationTypeText;
    int MaxLevel;                   // Application Max. Level (given by IDICT)

    std::shared_ptr<Logic::SourceBuffer> m_AppTknSource;

private:
    std::vector<FLOW*> m_flows_pre80;
};
