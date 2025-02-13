#pragma once

#include <Zsrcmgro/zSrcMgrO.h>
#include <Zsrcmgro/Compiler.h>
#include <zLogicO/Symbol.h>


class CLASS_DECL_ZSRCMGR SymbolAnalysisCompiler : public CCompiler
{
public:
    struct SymbolUse
    {
        std::wstring compilation_unit_name;
        std::wstring proc_name;
        std::wstring logic_line;
        int line_number_in_proc;
        int adjusted_line_number;
    };

public:
    SymbolAnalysisCompiler(Application& application);

    void Compile();

    const std::map<const Symbol*, std::vector<SymbolUse>>& GetSymbolUseMap() const { return m_symbolUseMap; }

private:
    Application& m_application;
    std::map<const Symbol*, std::vector<SymbolUse>> m_symbolUseMap;
};
