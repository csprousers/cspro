#include "StdAfx.h"
#include "ResourcePropertiesPageDlg.h"
#include <zUtilO/SpecialDirectoryLister.h>


BEGIN_MESSAGE_MAP(ResourcePropertiesPageDlg, CDialog)
    ON_BN_CLICKED(IDC_CALCULATE_RESOURCE_FILES, OnCalculateResourceFiles)
END_MESSAGE_MAP()


unsigned int ResourcePropertiesPageDlg::GetDialogTemplateId()
{
    return IDD_PAGE_PROPERTIES_RESOURCE;
}


ResourcePropertiesPageDlg::ResourcePropertiesPageDlg(AppResource resource, CWnd* const pParent/* = nullptr*/)
    :   CDialog(GetDialogTemplateId(), pParent),
        m_resource(std::move(resource)),
        m_type(PortableFunctions::FileIsDirectory(m_resource.GetPath()) ? 0 : 1),
        m_includeInCompiledApplication(m_resource.GetIncludeInCompiledApplication()),
        m_recursive(m_resource.GetRecursive()),
        m_filenameFilter(m_resource.GetFilenameFilter())
{
}


void ResourcePropertiesPageDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_DIRECTORY, m_type);
    DDX_Check(pDX, IDC_INCLUDE_IN_COMPILED_APPLICATION, m_includeInCompiledApplication);
    DDX_Check(pDX, IDC_RECURSIVE, m_recursive);
    DDX_Text(pDX, IDC_FILTER, m_filenameFilter);

    if( pDX->m_bSaveAndValidate )
    {
        m_resource.SetIncludeInCompiledApplication(( m_includeInCompiledApplication != 0 ));
        m_resource.SetRecursive(( m_recursive != 0 ));
        m_resource.SetFilenameFilter(m_filenameFilter);
    }
}


BOOL ResourcePropertiesPageDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    if( IsDirectory() )
    {
        GetDlgItem(IDC_FILE)->EnableWindow(FALSE);
    }

    else
    {
        GetDlgItem(IDC_DIRECTORY)->EnableWindow(FALSE);

        // hide the directory-specific controls
        for( const int resource_id : { IDC_RECURSIVE,
                                       IDC_FILTER_TEXT,
                                       IDC_FILTER,
                                       IDC_CALCULATE_RESOURCE_FILES } )
        {
            GetDlgItem(resource_id)->ShowWindow(SW_HIDE);
        }
    }

    return result;
}


void ResourcePropertiesPageDlg::OnCalculateResourceFiles()
{
    ASSERT(IsDirectory());

    UpdateData(TRUE);

    std::string text;

    try
    {
        const std::vector<std::string> file_paths = m_resource.GetEvaluatedPaths(false);

        text = FormatText("%d file%s", static_cast<int>(file_paths.size()), PluralizeWord(file_paths.size()));

        // show the file paths relative to the directory
        if( !file_paths.empty() )
        {
            text.append(":\r\n\r\n");

            const std::string fake_file_path = Path::Combine(m_resource.GetPath(), "g");

            text.append(SO::CreateSingleStringUsingCallback(file_paths,
                                                            [&](const std::string& file_path) { return GetRelativePathForDisplay(fake_file_path, file_path); },
                                                            SO::Newline_crlf_sv));
        }
    }

    catch( const CSProException& exception )
    {
        text = exception.what();
    }

    CWnd* const text_wnd = GetDlgItem(IDC_RESOURCE_FILES);
    text_wnd->SetWindowText(TC::ToWide(text).c_str());
    text_wnd->ShowWindow(SW_SHOW);
}


void ResourcePropertiesPageDlg::OnValidatePage()
{
    UpdateData(TRUE);

    // ensure that the directory or file exists
    if( !PortableFunctions::FileExists(m_resource.GetPath()) )
        throw CSProException("The resource path does not exist: " + m_resource.GetPath());

    // validate the filename filter
    SpecialDirectoryLister::EvaluateFilter(m_resource.GetFilenameFilter());
}
