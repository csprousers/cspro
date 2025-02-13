//------------------------------------------------------------------------------
// COMPALL.cpp  manage compiler functions
//------------------------------------------------------------------------------

#include <engine/StandardSystemIncludes.h>
#include <engine/Exappl.h>
#define ZSRCMGR_IMPL
#include <Zsrcmgro/Compiler.h>
#include <engine/COMPILAD.H>
#include <engine/Engine.h>
#include <zToolsO/Tools.h>
#include <zAppO/Application.h>
#include <zLogicO/SourceBuffer.h>

#include <engine/Ctab.h>


bool CEngineCompFunc::compobjOk(Symbol* objp)
{
    bool bApplCompile = objp->IsA(SymbolType::Application);

    // RHF INIC Nov 11, 2004
    if( bApplCompile )
    {
        CompileDictRelations();
        CompileExternalCode();

        if( GetSyntErr() != 0 )
            return false;
    }
    // RHF END Nov 11, 2004

    int isym = objp->GetSymbolIndex();

    if( isym < 0 )
        return true; // RHF Jan 27, 2000

    CString csObjName = WS2CS(NPT(isym)->GetName()); // RHF 03/12/99

    if( is_digit(csObjName[0]) )
        return true;                    // skip VIEW symbols


    // check if there is a PROC for this object
    const Logic::ProcDirectoryEntry* proc_directory_entry = GetProcDirectoryEntry(isym);

    if( proc_directory_entry == nullptr )
    {
        // because this is coming from the designer, we should only be compiling valid PROCS
        ASSERT(false);
        return true;
    }

    SetSourceBuffer(Appl.m_AppTknSource, proc_directory_entry);

    clearSyntaxErrorStatus();

    try
    {
        if( rutasync( objp->GetSymbolIndex() ) )
            ReportError(GetSyntErr());
    }
    catch(...) { ASSERT(false); }

    if( bApplCompile )
    {
        CompileReports();

        if( GetSyntErr() != 0 )
            return false;
    }


    ClearSourceBuffer();

    //BUCEN
    // CHK Jan 27
    if( CCompiler::GetCurrentSession() != nullptr )
    {
        const auto& parser_messages = CCompiler::GetCurrentSession()->GetParserMessages();
        const auto& error_check = std::find_if(parser_messages.cbegin(), parser_messages.cend(),
                                               [](const Logic::ParserMessage& parser_message) { return ( parser_message.type == Logic::ParserMessage::Type::Error ); });

        if( error_check != parser_messages.end() )
        {
            return false;
        }

        // este else evita hacer el chequeo que viene a continuacion
        // que solo se hace para volver a establecer bRet en false
        else if( GetSyntErr() != 0 )
        {
            return false;
        }
    }

    return true;
}
