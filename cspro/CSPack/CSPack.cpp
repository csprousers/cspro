#include "StdAfx.h"
#include "CSPack.h"
#include "PackDlg.h"
#include <zUtilO/ApplicationLoadException.h>


namespace
{
    // a command line processor to check for the /pack flag
    struct CSPackCommandLineProcessor : public CCommandLineInfo
    {
        bool pack = false;

        void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) override
        {
            if( bFlag && SO::EqualsNoCase(pszParam, _T("pack")) )
            {
                pack = true;
            }

            else
            {
                CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
            }
        }
    };
}


// The one and only CCSPackApp object
CCSPackApp theApp;


CCSPackApp::CCSPackApp()
    :   m_hAccelerators(nullptr)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CCSPackApp::InitInstance()
{
    CWinApp::InitInstance();

    AfxEnableControlContainer();

    SetRegistryKey(_T("U.S. Census Bureau"));

    CSPackCommandLineProcessor command_line_info;
    ParseCommandLine(command_line_info);

    std::unique_ptr<PackSpec> pack_spec;
    std::string file_path;

    if( !SO::IsWhitespace(command_line_info.m_strFileName) )
    {
        std::tie(pack_spec, file_path) = ProcessCommandLine(UTF8_TODO::GetUtf8(command_line_info.m_strFileName), command_line_info.pack);

        // if nothing is returned, that means that the command line was successfully processed
        // and we should quit the program
        if( pack_spec == nullptr )
            return FALSE;
    }

    // add the accelerators to the dialog
    m_hAccelerators = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_PACK));

    // run in interactive mode
    PackDlg dlg(std::move(pack_spec), std::move(file_path));
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}


BOOL CCSPackApp::ProcessMessageFilter(const int iCode, const LPMSG lpMsg)
{
    if( iCode >= 0 && m_pMainWnd != nullptr && m_hAccelerators != nullptr )
    {
        if( ::TranslateAccelerator(m_pMainWnd->m_hWnd, m_hAccelerators, lpMsg) )
            return TRUE;
    }

    return CWinApp::ProcessMessageFilter(iCode, lpMsg);
}


std::tuple<std::unique_ptr<PackSpec>, std::string> CCSPackApp::ProcessCommandLine(const std::string& file_path, const bool pack_flag)
{
    // options can come in on the command line as:
    //
    // (1) file path with .cspack
    //     - if pack flag is set, run the pack
    //     - otherwise open the file in interactive mode
    // (2) file path with .pff
    //     - run it if 8.0+, or if the Silent flag is set
    //     - otherwise open the file in interactive mode
    // (3) another file path
    //     - if pack flag is set, create a PackSpec for the file and run the pack
    //     - otherwise open the PackSpec in interactive mode

    try
    {
        std::unique_ptr<PackSpec> pack_spec;
        std::unique_ptr<PFF> pff;

        std::string file_path_to_return;
        bool run_pack = false;

        const std::string extension = PortableFunctions::PathGetFileExtension(file_path);

        // (1) file path with .cspack
        if( SO::EqualsNoCase(extension, FileExtensions::PackSpec) )
        {
            pack_spec = std::make_unique<PackSpec>();
            pack_spec->Load(file_path, pack_flag, pack_flag);

            file_path_to_return = file_path;
            run_pack = pack_flag;
        }

        // (2) file path with .pff
        else if( SO::EqualsNoCase(extension, FileExtensions::Pff) )
        {
            pff = std::make_unique<PFF>(UTF8_TODO::GetCString(file_path));

            if( !pff->LoadPifFile() )
                throw ApplicationFileLoadException(file_path);

            run_pack = ( GetCSProVersionNumeric(UTF8_TODO::GetUtf8(pff->GetVersion())) < PackSpec::VersionNewPackIntroduced ) ? pff->GetSilent() :
                                                                                                                                true;

            pack_spec = std::make_unique<PackSpec>(PackSpec::CreateFromPff(*pff, run_pack, run_pack));
        }

        // (3) another file path
        else
        {
            pack_spec = std::make_unique<PackSpec>();
            pack_spec->AddEntry(PackEntry::Create(file_path));
            pack_spec->SetZipFilePath(PortableFunctions::PathReplaceFileExtension(file_path, FileExtensions::Zip));

            run_pack = pack_flag;
        }

        // either return the pack specification for interactive mode...
        ASSERT(pack_spec != nullptr);

        if( !run_pack )
        {
            return std::make_tuple(std::move(pack_spec), std::move(file_path_to_return));
        }

        // ...or run the pack
        else
        {
            Packer().Run(pff.get(), *pack_spec);

            if( pff != nullptr )
                pff->ExecuteOnExitPff();
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return { nullptr, std::string() };
}
