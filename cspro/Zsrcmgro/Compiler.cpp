// Compiler.cpp: implementation of the CCompiler class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "Compiler.h"
#include "DesignerApplicationLoader.h"
#include "SrcCode.h"
#include <engine/Exappl.h>
#include <engine/Comp.h>
#include <zAppO/Application.h>
#include <zMessageO/MessageManager.h>
#include <zCapiO/CapiLogicParameters.h>
#include <zCapiO/CapiQuestionManager.h>
#include <ZTBDO/cLinkSub.h>
#include <ZTBDO/cLinkTab.h>


static bool bRunning = false;
static CCompiler* pThis = NULL;


CCompiler::CCompiler(Application* pApplication, CompilerCreator* compiler_creator/* = nullptr*/)
    :   m_CompIFaz(pApplication, compiler_creator)
{
    ASSERT( !bRunning );

    m_pApplication = pApplication;
    m_bFullCompile = false;
    m_bOptimizeFlowTree = false;
    m_bInit = false;
    m_csProcName.Empty();
    m_iLineNumberOfCurrentCompile = 0;

    bRunning = true;
    pThis = this;

    m_pApplication->SetApplicationLoader(std::make_shared<DesignerApplicationLoader>(m_pApplication, this));
}


CCompiler::~CCompiler()
{
    End();
    bRunning = false;
    pThis = NULL;
}


CCompiler::Result CCompiler::Init()
{
    if( m_bInit )
        return Result::NoInit;

    ClearParserMessages();

    // RHF INIC Jun 12, 2003
    CSourceCode* pSourceCode=NULL;
    ASSERT( m_pApplication != NULL && m_pApplication->GetAppSrcCode() != NULL );
    CString csLines;
    CString* pcsLines=NULL;

    if( m_pApplication != NULL && (pSourceCode=m_pApplication->GetAppSrcCode() ) != NULL )
    {
        CStringArray aProcLines;
        pSourceCode->GetProc( aProcLines );
        pSourceCode->ArrayToString( &aProcLines, csLines, true );
        pcsLines = &csLines;
    }
    // RHF END Jun 12, 2003

    bool bSomeError = false;
    m_pApplication->SetOptimizeFlowTree(this->m_bOptimizeFlowTree);

    if( !m_CompIFaz.C_CompilerInit( pcsLines, bSomeError ) )
        return Result::CantInit;

    m_bInit = true;

    return bSomeError ? Result::SomeErrors : Result::NoErrors;// RHF Mar 06, 2001
}


void CCompiler::End()
{
    m_CompIFaz.C_CompilerEnd();
    m_bInit = FALSE;
    m_pApplication->SetApplicationLoader(nullptr);
}


CCompiler::Result CCompiler::FullCompile(CSourceCode* pSourceCode)
{
    CCompiler::Result eErr = Result::NoErrors;

    if( !m_bInit && ( eErr = Init() ) != Result::NoErrors )
        return eErr;

    // before compiling the logic, report any parser messages coming from the message file
    CCompiler::Result eSomeErr = AddUserMessageParserMessages();

    // compile the logic
    ASSERT(pSourceCode != NULL);

    CString csAppSymb = _T("GLOBAL");
    CStringArray csaSymbProcs;
    CStringArray csaProc;

    pSourceCode->GetProcNames(csaSymbProcs);

    m_bFullCompile = true;

    //Compile the application procedure
    pSourceCode->GetProc(csaProc, csAppSymb);

    // create a blank PROC GLOBAL if necessary to force a compilation, as that triggers the compilation of external code
    if( csaProc.IsEmpty() )
        csaProc.Add(_T("PROC GLOBAL"));

    if( ( eErr = Compile(csAppSymb, csaProc) ) != Result::NoErrors )
        eSomeErr = eErr;

    //Compile the rest of the procedures
    for( int i = 0; i < csaSymbProcs.GetSize(); i++ )
    {
        CString csSymbName = csaSymbProcs.GetAt(i); // RHF Sep 14, 2000

        if( csSymbName.CompareNoCase( csAppSymb ) != 0 )
        {
            csaProc.RemoveAll();
            pSourceCode->GetProc( csaProc, csSymbName );

            CEventCode* pEvent = pSourceCode->GetEvent(csSymbName, CSourceCode_ProcProc);

            // + 1 because error message line numbers are 1-based
            m_iLineNumberOfCurrentCompile = ( pEvent != nullptr ) ? ( pEvent->GetEventLine() + 1 ) : 0;

            if( ( eErr = Compile( csSymbName, csaProc ) ) != Result::NoErrors )
                eSomeErr = eErr;
        }
    }

    m_bFullCompile = false;

    m_CompIFaz.m_pEngineDriver->m_pEngineCompFunc->CheckUnusedFileNames();

    return eSomeErr;
}


CCompiler::Result CCompiler::CompileExternalLogicOnly()
{
    CCompiler::Result eErr = Result::NoErrors;

    if( !m_bInit && ( eErr = Init() ) != Result::NoErrors )
        return eErr;

    CString csAppSymb = _T("GLOBAL");
    CStringArray csaProc;

    // compile with a blank application procedure
    csaProc.Add(_T("PROC GLOBAL"));

    return Compile(csAppSymb, csaProc);
}


CCompiler::Result CCompiler::CompileReport(const NamedTextSource& report_named_text_source)
{
    CCompiler::Result eErr = Result::NoErrors;

    if( !m_bInit && ( eErr = Init() ) != Result::NoErrors )
        return eErr;

    // compile the application procedure (which will compile the reports at the end)
    CSourceCode* pSourceCode = m_pApplication->GetAppSrcCode();
    CString csAppSymb = _T("GLOBAL");
    CStringArray csaProc;
    pSourceCode->GetProc(csaProc, csAppSymb);

    return Compile(csAppSymb, csaProc);
}


CCompiler::Result CCompiler::Compile(CapiQuestionManager& question_manager)
{
    question_manager.CompileCapiLogic(
        [&](const CapiLogicParameters& params)
        {
            return m_CompIFaz.m_pEngineCompFunc->CompileCapiLogic(params);
        });

    return ( m_CompIFaz.m_pEngineCompFunc->getErrors() == 0 ) ? Result::NoErrors : Result::SomeErrors;
}


CCompiler::Result CCompiler::Compile(const CString& csSymbName, const CStringArray& source_array)
{
    CCompiler::Result eErr = Result::NoErrors;

    if( csSymbName.IsEmpty() || source_array.IsEmpty() )
        return eErr;

    if( !m_bInit && ( eErr = Init() ) != Result::NoErrors )
        return eErr;

    // convert the source array into a single buffer and then a source buffer
    CString buffer_text;
    CSourceCode::ArrayToString(&source_array, buffer_text, true);

    m_csProcName = csSymbName;

    if ( !m_CompIFaz.C_CompilerCompile(buffer_text) )
        eErr = Result::SomeErrors;

    m_csProcName.Empty();
    m_iLineNumberOfCurrentCompile = 0;

    if( !m_bFullCompile )
        End();

    return eErr;
}


CCompiler* CCompiler::GetCurrentSession()
{
    return pThis;
}


int CCompiler::GetLinkTables( CArray<CLinkTable*,CLinkTable*>& aLinkTables )
{
    ASSERT( m_CompIFaz.m_pEngineDriver->m_pEngineCompFunc != NULL );

    int iNumLinkTables = m_CompIFaz.C_GetLinkTables( aLinkTables );

    return iNumLinkTables;
}


CCompiler::Result CCompiler::AddUserMessageParserMessages()
{
    const MessageFile& user_message_file = m_CompIFaz.m_pEngineDriver->GetUserMessageManager().GetMessageFile();

    Result result = Result::NoErrors;

    for( const Logic::ParserMessage& parser_message : user_message_file.GetLoadParserMessages() )
    {
        AddParserMessage(parser_message);

        if( parser_message.type == Logic::ParserMessage::Type::Error )
            result = Result::SomeErrors;
    }

    return result;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      CCompiler::GetIntDriver()
//
/////////////////////////////////////////////////////////////////////////////////
CIntDriver* CCompiler::GetIntDriver()
{
    return m_CompIFaz.m_pEngineDriver->m_pIntDriver.get();
}
