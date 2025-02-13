#pragma once

#include <zLogicO/Symbol.h>
#include <engine/Defines.h>
#include <zEngineO/AllSymbolDeclarations.h>

namespace Logic { class SourceBuffer; }


class CSymbolApl : public Symbol
{
public:
    CSymbolApl()
        :   Symbol(_T("GLOBAL"), SymbolType::Application),
            ApplicationType(ModuleType::None),
            MaxLevel(0)
    {
    }

    void AddFlow(FLOW* flow)               { m_flows_pre80.push_back(flow); }
    int GetNumFlows() const                { return (int)m_flows_pre80.size(); }
    FLOW* GetFlowAt(int flow_number) const { return m_flows_pre80[flow_number]; }

    void SetAppFileName(const CString& application_filename) { m_applicationFilename = application_filename; }
    const CString& GetAppFileName() const                    { return m_applicationFilename; }

public:
    ModuleType ApplicationType;     // Application Type
    CString ApplicationTypeText;
    int MaxLevel;                   // Application Max. Level (given by IDICT)

    std::shared_ptr<Logic::SourceBuffer> m_AppTknSource;

private:
    std::vector<FLOW*> m_flows_pre80;
    CString m_applicationFilename;
};
