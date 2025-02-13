#include "StdAfx.h"
#include "GlobalSettingsDlg.h"
#include <zUtilO/FileUtil.h>


BEGIN_MESSAGE_MAP(GlobalSettingsDlg, CDialog)
    ON_BN_CLICKED(IDC_HHC_BROWSE, OnBrowseHtmlHelpCompiler)
    ON_BN_CLICKED(IDC_WKHTMLTOPDF_BROWSE, OnBrowseWkhtmltopdf)
    ON_BN_CLICKED(IDC_CSPRO_CODE_BROWSE, OnBrowseCSProCode)
END_MESSAGE_MAP()


GlobalSettingsDlg::GlobalSettingsDlg(GlobalSettings global_settings, CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_GLOBAL_SETTINGS, pParent),
        m_globalSettings(std::move(global_settings)),
        m_automaticallyAssociateDocumentsWithDocSets(m_globalSettings.automatically_associate_documents_with_doc_sets)
#ifdef HELP_TODO_RESTORE_FOR_CSPRO81 
        m_buildDocumentsOnOpen(m_globalSettings.build_documents_on_open)
#endif
{
#ifdef HELP_TODO_RESTORE_FOR_CSPRO81 
    if( m_globalSettings.automatic_compilation_seconds != 0 )
        m_automaticCompilationSeconds = CS2WS(IntToString(m_globalSettings.automatic_compilation_seconds));
#endif
}


void GlobalSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_AUTOMATICALLY_ASSOCIATE_DOCUMENTS_WITH_DOCSETS, m_automaticallyAssociateDocumentsWithDocSets);
#ifdef HELP_TODO_RESTORE_FOR_CSPRO81 
    DDX_Check(pDX, IDC_BUILD_DOCUMENTS_ON_OPEN, m_buildDocumentsOnOpen);
    DDX_Text(pDX, IDC_AUTOMATIC_COMPILATION_SECONDS, m_automaticCompilationSeconds, true);
#endif
    DDX_Text(pDX, IDC_HHC_PATH, m_globalSettings.html_help_compiler_path, true);
    DDX_Text(pDX, IDC_WKHTMLTOPDF_PATH, m_globalSettings.wkhtmltopdf_path, true);
    DDX_Text(pDX, IDC_CSPRO_CODE_PATH, m_globalSettings.cspro_code_path, true);
}


void GlobalSettingsDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        m_globalSettings.automatically_associate_documents_with_doc_sets = ( m_automaticallyAssociateDocumentsWithDocSets != 0 );
#ifdef HELP_TODO_RESTORE_FOR_CSPRO81 
        m_globalSettings.build_documents_on_open = ( m_buildDocumentsOnOpen != 0 );

        // validate the seconds
        if( m_automaticCompilationSeconds.empty() )
        {
            m_globalSettings.automatic_compilation_seconds = 0;
        }

        else
        {
            const int64_t seconds = CIMSAString::IsNumeric(m_automaticCompilationSeconds) ? CIMSAString::Val(m_automaticCompilationSeconds) :
                                                                                            -1;
            if( seconds < 0 )
                throw CSProException(_T("The seconds value '%s' is not valid."), m_automaticCompilationSeconds.c_str());

            m_globalSettings.automatic_compilation_seconds = static_cast<unsigned>(seconds);
        }
#endif

        // validate the paths
        auto validate_path = [](const std::wstring& path, const bool is_directory)
        {
            if( !path.empty() && !( is_directory ? PortableFunctions::FileIsDirectory(path) :
                                                   PortableFunctions::FileIsRegular(path) ) )
            {
                throw CSProException(_T("The path does not exist: ") + path);
            }
        };

        validate_path(m_globalSettings.html_help_compiler_path, false);
        validate_path(m_globalSettings.wkhtmltopdf_path, false);
        validate_path(m_globalSettings.cspro_code_path, true);

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void GlobalSettingsDlg::OnBrowseFile(std::wstring& path, const TCHAR* path_type)
{
    UpdateData(TRUE);

    CIMSAFileDialog file_dlg(TRUE, nullptr, path.c_str(), OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                             _T("Executables (*.exe)|*.exe|All Files (*.*)|*.*||"), this, CFD_NO_DIR);

    const std::wstring title = FormatTextCS2WS(_T("Select the %s Executable"), path_type);
    file_dlg.m_ofn.lpstrTitle = title.c_str();

    if( file_dlg.DoModal() != IDOK )
        return;

    path = CS2WS(file_dlg.GetPathName());

    UpdateData(FALSE);
}


void GlobalSettingsDlg::OnBrowseHtmlHelpCompiler()
{
    OnBrowseFile(m_globalSettings.html_help_compiler_path, _T("Microsoft HTML Help Compiler"));
}


void GlobalSettingsDlg::OnBrowseWkhtmltopdf()
{
    OnBrowseFile(m_globalSettings.wkhtmltopdf_path, _T("wkhtmltopdf"));
}


void GlobalSettingsDlg::OnBrowseCSProCode()
{
    UpdateData(TRUE);

    std::optional<std::wstring> folder = SelectFolderDialog(m_hWnd, _T("Select the CSPro Code Folder"), m_globalSettings.cspro_code_path.c_str());

    if( !folder.has_value() )
        return;

    m_globalSettings.cspro_code_path = std::move(*folder);

    UpdateData(FALSE);
}
