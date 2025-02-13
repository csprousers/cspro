#include "StdAfx.h"
#include "SymbolAnalysisCompiler.h"
#include "SrcCode.h"
#include <zAppO/Application.h>
#include <engine/Comp.h>
#include <engine/CompilerCreator.h>


namespace
{
    // only keep track of dictionary/form symbols, as well as user-defined functions
    std::vector<SymbolType> SymbolTypesToProcess =
    {
        SymbolType::Pre80Dictionary,
        SymbolType::Section,
        SymbolType::Variable,
        SymbolType::ValueSet,

        SymbolType::Pre80Flow, // FLOW_TODO add support for the new flow
        SymbolType::Form,
        SymbolType::Group,
        SymbolType::Block,

        SymbolType::UserFunction
    };


    class SymbolAnalysisCompilerEngineCompFunc : public CEngineCompFunc
    {
    public:
        SymbolAnalysisCompilerEngineCompFunc(CEngineDriver* pEngineDriver)
            :   CEngineCompFunc(pEngineDriver),
                m_symbolUseMap(nullptr)
        {
        }

        void Initialize(Application& application, std::map<const Symbol*, std::vector<SymbolAnalysisCompiler::SymbolUse>>& symbol_use_map)
        {
            m_procLineNumberMap = application.GetAppSrcCode()->GetProcLineNumberMap();
            m_symbolUseMap = &symbol_use_map;
        }

    private:
        void ProcessSymbol() override
        {
            CEngineCompFunc::ProcessSymbol();

            // only process certain symbols
            const Symbol* symbol = CurrentToken.symbol;

            if( !symbol->IsOneOf(SymbolTypesToProcess) ||
                symbol->GetSubType() == SymbolSubType::DynamicValueSet )
            {
                return;
            }

            const Logic::BasicToken* basic_token = GetCurrentBasicToken();

            ASSERT(m_symbolUseMap != nullptr && basic_token != nullptr);

            auto symbol_use_lookup = m_symbolUseMap->find(symbol);
            auto& symbol_uses = ( symbol_use_lookup != m_symbolUseMap->end() ) ? symbol_use_lookup->second :
                                                                                 m_symbolUseMap->try_emplace(symbol).first->second;

            std::wstring proc_name = GetCurrentProcName();
            int line_number_in_proc = (int)basic_token->line_number;

            // don't add multiple entries for the same line
            const auto& entry_lookup = std::find_if(symbol_uses.cbegin(), symbol_uses.cend(),
                [&](const auto& su)
                {
                    return ( su.proc_name == proc_name && su.line_number_in_proc == line_number_in_proc );
                });

            if( entry_lookup != symbol_uses.cend() )
                return;

            auto& symbol_use = symbol_uses.emplace_back(
                SymbolAnalysisCompiler::SymbolUse
                {
                    GetCurrentCompilationUnitName(),
                    proc_name,
                    GetBasicTokenLine(*basic_token),
                    line_number_in_proc,
                    line_number_in_proc
                });

            // adjust the line number if this is a source-related message not from an external code file or a report
            if( symbol_use.line_number_in_proc != 0 && symbol_use.compilation_unit_name.empty() )
            {
                const auto& line_number_lookup = m_procLineNumberMap.find(symbol_use.proc_name);

                if( line_number_lookup != m_procLineNumberMap.cend() )
                    symbol_use.adjusted_line_number += line_number_lookup->second;
            }
        }

    private:
        std::map<std::wstring, int> m_procLineNumberMap;
        std::map<const Symbol*, std::vector<SymbolAnalysisCompiler::SymbolUse>>* m_symbolUseMap;
    };


    class SymbolAnalysisCompilerEngineCompFuncCreator : public CompilerCreator
    {
    public:
        std::unique_ptr<CEngineCompFunc> CreateCompiler(CEngineDriver* pEngineDriver) override
        {
            return std::make_unique<SymbolAnalysisCompilerEngineCompFunc>(pEngineDriver);
        }
    };
}


SymbolAnalysisCompiler::SymbolAnalysisCompiler(Application& application)
    :   CCompiler(&application, std::make_unique<SymbolAnalysisCompilerEngineCompFuncCreator>().get()),
        m_application(application)
{
}


void SymbolAnalysisCompiler::Compile()
{
    try
    {
        SymbolAnalysisCompilerEngineCompFunc* compiler = assert_cast<SymbolAnalysisCompilerEngineCompFunc*>(GetEngineDriver()->m_pEngineCompFunc.get());

        compiler->Initialize(m_application, m_symbolUseMap);

        FullCompile(m_application.GetAppSrcCode());
    }
    catch(...) { ASSERT(false); }

    for( const Logic::ParserMessage& parser_message : GetParserMessages() )
    {
        if( parser_message.type == Logic::ParserMessage::Type::Error )
            throw CSProException(_T("Compilation error: %s"), parser_message.message_text.c_str());
    }
}
