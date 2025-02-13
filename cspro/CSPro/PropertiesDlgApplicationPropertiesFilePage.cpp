#include "StdAfx.h"
#include "PropertiesDlgApplicationPropertiesFilePage.h"


BEGIN_MESSAGE_MAP(PropertiesDlgApplicationPropertiesFilePage, CDialog)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_SAVE_PROPERTIES_TO_APPLICATION, IDC_SAVE_PROPERTIES_TO_CSPROPS, OnUseFileChange)
    ON_BN_CLICKED(IDC_SELECT_CSPROPS_FILE, OnSelectFile)
END_MESSAGE_MAP()


PropertiesDlgApplicationPropertiesFilePage::PropertiesDlgApplicationPropertiesFilePage(const Application& application, CWnd* pParent/* = nullptr*/)
    :   CDialog(PropertiesDlgApplicationPropertiesFilePage::IDD, pParent),
        m_application(application),
        m_applicationPropertiesFilename(application.GetApplicationPropertiesFilename()),
        m_applicationFilename(CS2WS(application.GetApplicationFilename())),
        m_useApplicationPropertiesFile(m_applicationPropertiesFilename.empty() ? 0 : 1)
{
}


void PropertiesDlgApplicationPropertiesFilePage::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_SAVE_PROPERTIES_TO_APPLICATION, m_useApplicationPropertiesFile);
    DDX_Text(pDX, IDC_APPLICATION_FILENAME, m_applicationFilename);
    DDX_Text(pDX, IDC_CSPROPS_FILENAME, m_applicationPropertiesFilename);
}


BOOL PropertiesDlgApplicationPropertiesFilePage::OnInitDialog()
{
    CDialog::OnInitDialog();

    EnableDisableControls();

    return TRUE;
}


void PropertiesDlgApplicationPropertiesFilePage::FormToProperties()
{
    UpdateData(TRUE);

    SO::MakeTrim(m_applicationPropertiesFilename);

    if( m_useApplicationPropertiesFile == 1 && m_applicationPropertiesFilename.empty() )
        throw CSProException("You must specify an Application Properties file.");
}


void PropertiesDlgApplicationPropertiesFilePage::ResetProperties()
{
    m_applicationPropertiesFilename = m_application.GetApplicationPropertiesFilename();
    m_useApplicationPropertiesFile = m_applicationPropertiesFilename.empty() ? 0 : 1;

    UpdateData(FALSE);
    EnableDisableControls();
}


void PropertiesDlgApplicationPropertiesFilePage::OnOK()
{
    FormToProperties();

    CDialog::OnOK();
}


void PropertiesDlgApplicationPropertiesFilePage::EnableDisableControls()
{
    BOOL enabled = ( m_useApplicationPropertiesFile == 1 );
    GetDlgItem(IDC_SELECT_CSPROPS_FILE)->EnableWindow(enabled);
}


void PropertiesDlgApplicationPropertiesFilePage::OnUseFileChange(UINT /*nID*/)
{
    UpdateData(TRUE);
    EnableDisableControls();
}


void PropertiesDlgApplicationPropertiesFilePage::OnSelectFile()
{
    UpdateData(TRUE);

    std::wstring filename = m_applicationPropertiesFilename;

    // if no filename is provided, base the default filename on the application filename
    if( filename.empty() )
        filename = PortableFunctions::PathRemoveFileExtension(m_applicationFilename) + FileExtensions::WithDot::ApplicationProperties;

    CIMSAFileDialog file_dlg(FALSE, FileExtensions::ApplicationProperties, filename.c_str(), OFN_HIDEREADONLY,
                             _T("Application Properties Files (*.csprops)|*.csprops||"), this);

    file_dlg.m_ofn.lpstrTitle = _T("Select Application Properties File");

    if( file_dlg.DoModal() != IDOK )
        return;

    std::wstring new_filename = CS2WS(file_dlg.GetPathName());

    if( !SO::EqualsNoCase(new_filename, m_application.GetApplicationPropertiesFilename()) && PortableFunctions::FileIsRegular(new_filename) )
    {
        try
        {
            // read the new application properties
            ApplicationProperties application_properties;
            application_properties.Open(new_filename);

            int result = AfxMessageBox(FormatText(_T("The file '%s' already exists. If you select this file, the current application properties will be ")
                                                  _T("discarded in favor of the properties in this file. Are you sure you want to use this file?"),
                                                  PortableFunctions::PathGetFilename(new_filename)),
                                       MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);

            if( result == IDYES )
            {
                // set them
                if( AfxGetMainWnd()->SendMessage(UWM::CSPro::SetExternalApplicationProperties,
                                                 reinterpret_cast<WPARAM>(&new_filename), reinterpret_cast<LPARAM>(&application_properties)) != 1 )
                {
                    throw CSProException("There was an error setting the application properties.");
                }

                // and then close the Application Properties dialog on success
                GetParent()->SendMessage(WM_CLOSE);
                return;
            }
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
        }

        new_filename.clear();
    }

    m_applicationPropertiesFilename = new_filename;

    UpdateData(FALSE);
}
