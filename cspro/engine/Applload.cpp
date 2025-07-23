//----------------------------------------------------------------------
//  AplLoad.cpp: load applications
//----------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "Tables.h"
#include "Comp.h"
#include "IntDrive.h"
#include <zToolsO/Tools.h>
#include <zUtilO/AppLoader.h>
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

    m_pEngineSettings->m_io_Dic.clear();
    m_pEngineSettings->m_io_Var.clear();
    m_pEngineSettings->m_io_Err = 0;
    m_pEngineSettings->m_failMessage.clear();

    // loading the contents of the Application' Dicts and Flows
    if( !LoadApplDics() || !LoadApplFlows() )
        return false;

    // Using binary -> No ascii loading anymore
    if( GetApplication()->GetAppLoader()->GetBinaryFileLoad() )
    {
        try
        {
            LoadCompiledBinary();
            return true;
        }

        catch( const std::exception& exception )
        {
            ErrorMessage::Display(FormatText(MGF::GetMessageText(MGF::ErrorReadingPen)->c_str(),
                                             Path::GetFilename(GetApplication()->GetAppLoader()->GetArchiveFilePath()).c_str(),
                                             exception.what()));
            return false;
        }
    }

#ifdef WIN_DESKTOP
    ASSERT(Appl.m_AppTknSource == nullptr);

    SharableString compiler_buffer;

    // load the buffer from passed in lines
    if( pcsLines != nullptr )
    {
        compiler_buffer = UTF8_TODO::GetUtf8(*pcsLines);
    }

    // or from the disk
    else
    {
        const CodeFile* const logic_main_code_file = GetApplication()->GetLogicMainCodeFile();

        if( logic_main_code_file != nullptr )
        {
            compiler_buffer = logic_main_code_file->GetTextSource().GetTextAsSharableString();
        }

        else
        {
            issaerror(MessageType::Error, 10058);
            return false;
        }
    }

    Appl.m_AppTknSource = std::make_unique<Logic::SourceBuffer>(std::move(compiler_buffer));

    // scan for tables so the symbols exist and will be valid for adding to the PROC directory
    if( Appl.ApplicationType != ModuleType::Entry && !m_pEngineCompFunc->ScanTables() )
        return false;

    //calling inittables after the ScanTables to get the number of tables dynamically. Do not clear the symbol table. This is a memory only allocation
    m_pEngineArea->inittables(false);
    if( Issamod != ModuleType::Designer )
    {
        m_pEngineCompFunc->SetSourceBuffer(Appl.m_AppTknSource);
        return ( m_pEngineCompFunc->CreateProcDirectory() != nullptr );
    }
#endif

    return true;
}


bool CEngineDriver::LoadApplMessage()
{
    // evaluate load done & prepare message
    if( m_pEngineSettings->m_io_Err == 0 )
    {
        // successful loading
        m_pEngineSettings->m_failMessage.clear();
    }

    else
    {
        // type of error
        const char* type;

        switch( m_pEngineSettings->m_io_Err )
        {
            case 1:  type = "name already defined"; break;
            case 2:  type = "no place to insert";   break;
            case 3:  type = "floats overflow";      break;
            case 91: type = "excessive indexing";   break; // victor Aug 25, 99
            case 92: type = "invalid ranges";       break; // RHF Nov 03, 2000
            default: type = "unable to load";       break;
        }

        // diagnostics message
        m_pEngineSettings->m_failMessage = FormatText("Cannot load %s: ", m_pEngineSettings->m_io_Dic.c_str());

        if( !m_pEngineSettings->m_io_Var.empty() )
        {
            m_pEngineSettings->m_failMessage.append("Var ")
                                            .append(m_pEngineSettings->m_io_Var)
                                            .append(" - ");
        }

        m_pEngineSettings->m_failMessage.append(type);
    }

    return m_pEngineSettings->m_failMessage.empty();
}
