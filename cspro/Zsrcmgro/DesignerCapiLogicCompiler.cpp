#include "StdAfx.h"
#include "DesignerCapiLogicCompiler.h"
#include "SrcCode.h"
#include "Wcompile.h"
#include <zAppO/Application.h>
#include <engine/Comp.h>


DesignerCapiLogicCompiler::DesignerCapiLogicCompiler(Application& application)
    :   BackgroundCompiler(application, this),
        m_application(application),
        m_procGlobalCompiled(false)
{
}


CEngineDriver* DesignerCapiLogicCompiler::GetEngineDriver()
{
    return m_compIFaz->m_pEngineDriver;
}


DesignerCapiLogicCompiler::CompileResult DesignerCapiLogicCompiler::Compile(const CapiLogicParameters& capi_logic_parameters)
{
    try
    {
        ClearParserMessages();

        // compile the contents of PROC GLOBAL
        if( !m_procGlobalCompiled )
        {
            m_procName = "GLOBAL";

            CStringArray proc_global_lines;
            CString proc_global_buffer;

            if( m_application.GetAppSrcCode() != nullptr )
                m_application.GetAppSrcCode()->GetProc(proc_global_lines, UTF8_TODO::GetCString(m_procName));

            CSourceCode::ArrayToString(&proc_global_lines, proc_global_buffer, true);

            BackgroundCompiler::Compile(std::make_unique<Logic::SourceBuffer>(UTF8_TODO::GetUtf8(proc_global_buffer)));

            m_procGlobalCompiled = true;
        }

        // compile the CAPI logic
        if( std::holds_alternative<int>(capi_logic_parameters.symbol_index_or_name) )
        {
            m_procName = NPT_Ref(std::get<int>(capi_logic_parameters.symbol_index_or_name)).GetName();
        }

        else
        {
            ASSERT(false);
            m_procName = std::get<std::string>(capi_logic_parameters.symbol_index_or_name);
        }

        m_compIFaz->m_pEngineCompFunc->CompileCapiLogic(capi_logic_parameters);
    }
    catch(...) { ASSERT(false); }

    CompileResult result;

    // only add error messages
    for( const Logic::ParserMessage& parser_message : GetParserMessages() )
    {
        if( parser_message.type == Logic::ParserMessage::Type::Error )
            result.errors.emplace_back(parser_message);
    }

    return result;
}
