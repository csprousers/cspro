#include "StdAfx.h"
#include "Wcompile.h"
#include "SrcCode.h"
#include <zLogicO/ProcDirectory.h>
#include <engine/CompIlad.h>
#include <engine/Ctab.h>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


CCompIFaz::CCompIFaz(Application* pApplication, CompilerCreator* compiler_creator/* = nullptr*/)
{
    m_pEngineDriver = new CEngineDriver(pApplication, false, compiler_creator);
    m_pEngineArea = m_pEngineDriver->m_pEngineArea;
    m_engineData = &m_pEngineArea->GetEngineData();
    m_pEngineCompFunc = m_pEngineDriver->m_pEngineCompFunc.get();
    m_pEngineSettings = m_pEngineDriver->m_pEngineSettings;
}


CCompIFaz::~CCompIFaz()
{
    delete m_pEngineDriver;
}


const Logic::SymbolTable& CCompIFaz::GetSymbolTable() const
{
    return m_pEngineArea->GetSymbolTable();
}


// Return TRUE/FALSE
bool CCompIFaz::C_CompilerInit( CString* pcsLines, bool& bSomeError )// RHF Jun 12, 2003 Add pcrLines
{
    bSomeError = false;
    m_pEngineDriver->InitAppName();
    m_pEngineArea->inittables();

    // load the messages
    m_pEngineDriver->BuildMessageManagers();

    if( !m_pEngineDriver->attrload() || m_pEngineSettings->m_io_Err != 0 )
    {
        C_CompilerEnd();
        return false;
    }

    if( !m_pEngineDriver->LoadApplChildren(pcsLines) ) // RHF Jun 12, 2003 Add pcrLines
    {
        issaerror(MessageType::Error, 10004, Path::GetFilename(m_pEngineDriver->GetApplication()->GetApplicationFilePath()).c_str(),
                                             m_pEngineSettings->m_failMessage.c_str());

        // RHF COM Mar 06, 2001 return false;
        bSomeError = true;
        return true;
    }

    WindowsDesktopMessage::Send(WM_IMSA_SYMBOLS_ADDED, 1, &m_pEngineArea->GetSymbolTable());

    return true;
}


void CCompIFaz::C_CompilerEnd()
{
    m_pEngineCompFunc->CheckProcTables();

    Appl.m_AppTknSource.reset();

    m_pEngineArea->tablesend();
}


bool CCompIFaz::C_CompilerCompile(std::shared_ptr<Logic::SourceBuffer> source_buffer)
{
    Appl.m_AppTknSource = std::move(source_buffer);
    m_pEngineDriver->m_pEngineCompFunc->SetSourceBuffer(Appl.m_AppTknSource);

    const Logic::ProcDirectory* const proc_directory = m_pEngineDriver->m_pEngineCompFunc->CreateProcDirectory();

    if( proc_directory == nullptr )
        return false;

    for( const auto& [symbol_index, proc_directory_entry] : proc_directory->GetEntries() )
    {
        if( !m_pEngineCompFunc->CompileProc(NPT(symbol_index)) )
            return false;
    }

    WindowsDesktopMessage::Send(WM_IMSA_SYMBOLS_ADDED, 0, &m_pEngineArea->GetSymbolTable());

    return true;
}


// SERPRO_CALC

int CCompIFaz::C_GetLinkTables( CArray<CLinkTable*,CLinkTable*>& aLinkTables )
{
    for( CTAB* pCtab : m_engineData->crosstabs )
    {
        if( !pCtab->IsAuxiliarTable() )
            aLinkTables.Add( pCtab->GetLinkTable() );
    }

    return aLinkTables.GetSize();
}

// SERPRO_CALC
