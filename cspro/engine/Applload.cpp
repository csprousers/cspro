//----------------------------------------------------------------------
//  AplLoad.cpp: load applications
//----------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "Tables.h"
#include "Engine.h"
#include "Comp.h"
#include "IntDrive.h"
#include <zToolsO/Tools.h>
#include <zUtilO/AppLdr.h>
#include <zAppO/Application.h>
#include <zMessageO/Messages.h>
#include <zLogicO/SourceBuffer.h>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


bool CEngineDriver::LoadApplChildren(CString* pcsLines)
{
    // Dicts/Flows/Flow'Forms symbols must be already inserted by
    // 'MakeApplChildren', previously called in 'attrload' (Attr.cpp)

    io_Dic.Empty();
    io_Var.Empty();
    io_Err = 0;
    Failmsg.Empty();

    // loading the contents of the Application' Dicts and Flows
    if( !LoadApplDics() || !LoadApplFlows() )
        return false;

    // getting the APP file
    Appl.SetAppFileName(m_csAppFullName);

    // Using binary -> No ascii loading anymore
    if( GetApplication()->GetAppLoader()->GetBinaryFileLoad() )
    {
        try
        {
            LoadCompiledBinary();
            return true;
        }

        catch(...)
        {
            ErrorMessage::Display(FormatText(MGF::GetMessageText(MGF::ErrorReadingPen).c_str(),
                                             GetApplication()->GetAppLoader()->GetArchiveName().c_str()));
            return false;
        }
    }

#ifdef WIN_DESKTOP
    ASSERT(Appl.m_AppTknSource == nullptr);

    std::wstring compiler_buffer;

    // load the buffer from passed in lines
    if( pcsLines != nullptr )
    {
        compiler_buffer = CS2WS(*pcsLines);
    }

    // or from the disk
    else
    {
        const CodeFile* logic_main_code_file = GetApplication()->GetLogicMainCodeFile();

        if( logic_main_code_file != nullptr )
        {
            compiler_buffer = logic_main_code_file->GetTextSource().GetText();
        }

        else
        {
            issaerror(MessageType::Error, 10058);
            return false;
        }
    }

    Appl.m_AppTknSource = std::make_shared<Logic::SourceBuffer>(std::move(compiler_buffer));

    // scan for tables so the symbols exist and will be valid for adding to the PROC directory
    if( Appl.ApplicationType != ModuleType::Entry && !m_pEngineCompFunc->ScanTables() )
        return false;

    if( Issamod != ModuleType::Designer )
    {
        m_pEngineCompFunc->SetSourceBuffer(Appl.m_AppTknSource);
        return ( m_pEngineCompFunc->CreateProcDirectory() != nullptr );
    }
#endif

    return true;
}

bool CEngineDriver::LoadApplMessage( void ) {
    // evaluate load done & prepare message
    bool    bLoadOK = ( !io_Err );

    if( bLoadOK ) {
        Failmsg.Empty(); // successful loading
    }
    else {
        // looks for source of error
        CString csObjError;
        CString csExplain;

        if( !io_Var.IsEmpty() )
            csObjError.Format( _T("Var %s"), io_Var.GetString() );

        // type of error
        if( io_Err == 1 )
            csExplain = _T("name already defined");
        else if( io_Err == 2 )
            csExplain = _T("no place to insert");
        else if( io_Err == 3 )
            csExplain = _T("floats overflow");
        else if( io_Err == 91 )                      // victor Aug 25, 99
            csExplain = _T("excessive indexing");    // victor Aug 25, 99
        else if( io_Err == 92 )                      // victor Aug 25, 99
            csExplain = _T("invalid ranges");        // RHF Nov 03, 2000
        else
            csExplain = _T("unable to load");

        // diagnostics message
        Failmsg.Format(_T("Cannot load %s: "), io_Dic.GetString());
        if( !csObjError.IsEmpty() )
            Failmsg.AppendFormat(_T("%s - "), csObjError.GetString());
        Failmsg.Append(csExplain);
    }

    return bLoadOK;
}
