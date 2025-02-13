//------------------------------------------------------------------------------
// COMPALL.cpp  manage compiler functions
//------------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "Exappl.h"
#include "COMPILAD.H"
#include "Engine.h"
#include "Ctab.h"
#include <zToolsO/Tools.h>
#include <zAppO/Application.h>
#include <zListingO/ErrorLister.h>
#include <zLogicO/ProcDirectory.h>

#define RTRACE TRACE


int CEngineCompFunc::compall( int comptype )
{
    // comptype: 0 => all, 1 => APP, 2 => rest
    if( comptype == 0 || comptype == 1 )
    {
        Flagcomp = 1;
        resetErrors();

        CompileProc(&Appl);
    }

    if( comptype == 0 || comptype == 2 ) {
        // former dictrip was replaced by GroupTtrip    // victor Jul 26, 99
        // --- allowed procs: only for named, visible elements in Flows
        // --- no Dicttrip - section or record names have no proc anymore
        int     iNumberOfFlows = Appl.GetNumFlows();

        for( int iNumFlow = 0; iNumFlow < iNumberOfFlows; iNumFlow++ ) {
            ASSERT( Appl.GetFlowAt( iNumFlow ) );
            m_pEngineDriver->SetFlowInProcess( Appl.GetFlowAt( iNumFlow ) );
            int     iLevel = 0;
            GROUPT* pGroupT = m_pEngineDriver->GetGroupTRootInProcess()->GetLevelGPT( iLevel );

            m_pEngineArea->GroupTtrip( pGroupT, (pGroupTripFunc)&CEngineCompFunc::CompileProc );

            m_pEngineDriver->ResetFlowInProcess();
        }

        // don't replace with a range-based for loop because auxiliary tables may be added as part of the compilation
        for( size_t i = 0; i < m_engineData->crosstabs.size(); ++i )
        {
            CTAB* pCtab = m_engineData->crosstabs[i];

            if( !pCtab->IsAuxiliarTable() )
                CompileProc(pCtab);
        }

        if( getErrors() > 0 )
        {
            issaerror(MessageType::Abort, 10010, (LPCTSTR)PortableFunctions::PathGetFilename(Appl.GetAppFileName()), getErrors());
            return 1;
        }

        return getErrors();
    }

    return 0;
}


bool CEngineCompFunc::compobjOk(Symbol* symbol)
{
    if( Appl.m_AppTknSource == nullptr )
        return true;

    int symbol_index = symbol->GetSymbolIndex();

    if( symbol_index <= 0 )
    {
        ASSERT(false);
        return true;
    }

    ASSERT(symbol->IsOneOf(
        SymbolType::Application,
        SymbolType::Block,
        SymbolType::Crosstab,
        SymbolType::Group,
        SymbolType::Variable
    ));

    if( symbol->IsA(SymbolType::Application) )
    {
        // prior to PROC GLOBAL being processed...
        ASSERT(Prognext == 0);

        // ...compile any dictionary relations
        CompileDictRelations();

        // ...and compile external code files
        CompileExternalCode();
    }

    // check if there is a PROC for this object
    const Logic::ProcDirectoryEntry* proc_directory_entry = GetProcDirectoryEntry(symbol->GetSymbolIndex());

    if( proc_directory_entry != nullptr )
    {
        clearSyntaxErrorStatus();

        SetSourceBuffer(Appl.m_AppTknSource, proc_directory_entry);

        try
        {
            if( rutasync(symbol->GetSymbolIndex()) )
                ReportError(GetSyntErr());
        }
        catch(...) { ASSERT(false); }

        ClearSourceBuffer();

        if( GetSyntErr() != 0 || ( m_pEngineDriver->GetCompilerErrorLister() != nullptr && m_pEngineDriver->GetCompilerErrorLister()->HasErrors() ) )
            return false;
    }

    if( symbol->IsA(SymbolType::Application) )
    {
        // compile reports once PROC GLOBAL has been processed
        CompileReports();
    }

    return true;
}
