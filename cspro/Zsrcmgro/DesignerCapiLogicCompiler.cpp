#include "StdAfx.h"
#include "DesignerCapiLogicCompiler.h"
#include "SrcCode.h"
#include "Wcompile.h"
#include <zAppO/Application.h>
#include <engine/Comp.h>


DesignerCapiLogicCompiler::DesignerCapiLogicCompiler(Application& application)
    :   BackgroundCompiler(application, this),
        m_application(application)
{
}


CEngineDriver* DesignerCapiLogicCompiler::GetEngineDriver()
{
    return m_compIFaz->m_pEngineDriver;
}


DesignerCapiLogicCompiler::CompileResult DesignerCapiLogicCompiler::Compile(const CapiLogicParameters& capi_logic_parameters)
{
    int expression = -1;

    try
    {
        ClearParserMessages();

        // compile the contents of PROC GLOBAL
        m_procName = "GLOBAL";

        CStringArray proc_global_lines;
        CString proc_global_buffer;

        if( m_application.GetAppSrcCode() != nullptr )
            m_application.GetAppSrcCode()->GetProc(proc_global_lines, UTF8_TODO::GetCString(m_procName));

        CSourceCode::ArrayToString(&proc_global_lines, proc_global_buffer, true);

        BackgroundCompiler::Compile(std::make_unique<Logic::SourceBuffer>(UTF8_TODO::GetUtf8(proc_global_buffer)));

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

        expression = m_compIFaz->m_pEngineCompFunc->CompileCapiLogic(capi_logic_parameters);
    }
    catch(...) { ASSERT(false); }

    CompileResult result { expression };

    // only display the last error
    const std::vector<Logic::ParserMessage>& parser_messages = GetParserMessages();

    for( auto parser_message_itr = parser_messages.crbegin(); parser_message_itr != parser_messages.crend(); ++parser_message_itr )
    {
        if( parser_message_itr->type == Logic::ParserMessage::Type::Error )
        {
            result.error_message = parser_message_itr->message_text;
            break;
        }
    }

    return result;
}
