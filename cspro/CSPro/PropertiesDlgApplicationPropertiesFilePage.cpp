#include "StdAfx.h"
#include "PropertiesDlgApplicationPropertiesFilePage.h"


BEGIN_MESSAGE_MAP(PropertiesDlgApplicationPropertiesFilePage, CDialog)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_SAVE_PROPERTIES_TO_APPLICATION, IDC_SAVE_PROPERTIES_TO_CSPROPS, OnUseFileChange)
    ON_BN_CLICKED(IDC_SELECT_CSPROPS_FILE, OnSelectFile)
END_MESSAGE_MAP()


PropertiesDlgApplicationPropertiesFilePage::PropertiesDlgApplicationPropertiesFilePage(const Application& application, CWnd* const pParent/* = nullptr*/)
    :   CDialog(PropertiesDlgApplicationPropertiesFilePage::IDD, pParent),
        m_application(application),
        m_applicationPropertiesFilePath(application.GetApplicationPropertiesFilePath()),
        m_applicationFilePath(application.GetApplicationFilePath()),
        m_useApplicationPropertiesFile(m_applicationPropertiesFilePath.empty() ? 0 : 1)
{
}


const std::string& PropertiesDlgApplicationPropertiesFilePage::GetApplicationPropertiesFilePath() const
{
    return ( m_useApplicationPropertiesFile == 0 ) ? SO::Empty_string :
                                                     m_applicationPropertiesFilePath;
}


void PropertiesDlgApplicationPropertiesFilePage::DoDataExchange(CDataExchange* const pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_SAVE_PROPERTIES_TO_APPLICATION, m_useApplicationPropertiesFile);
    DDX_Text(pDX, IDC_APPLICATION_FILENAME, m_applicationFilePath);
    DDX_Text(pDX, IDC_CSPROPS_FILENAME, m_applicationPropertiesFilePath);
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

    SO::MakeTrim(m_applicationPropertiesFilePath);

    if( m_useApplicationPropertiesFile == 1 && m_applicationPropertiesFilePath.empty() )
        throw CSProException("You must specify an Application Properties file.");
}


void PropertiesDlgApplicationPropertiesFilePage::ResetProperties()
{
    m_applicationPropertiesFilePath = m_application.GetApplicationPropertiesFilePath();
    m_useApplicationPropertiesFile = m_applicationPropertiesFilePath.empty() ? 0 : 1;

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
    const BOOL enabled = ( m_useApplicationPropertiesFile == 1 );
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

    std::string file_path = m_applicationPropertiesFilePath;

    // if no file path is provided, base the default file path on the application file path
    if( file_path.empty() )
        file_path = PortableFunctions::PathReplaceFileExtension(m_applicationFilePath, FileExtensions::ApplicationProperties);

    SaveFileDlg save_file_dlg(OFN_HIDEREADONLY, FileExtensions::ApplicationProperties, file_path,
                             L"Application Properties Files (*.csprops)|*.csprops||", this);
    save_file_dlg.SetTitle(L"Select Application Properties File");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    std::string new_file_path = save_file_dlg.GetFilePath();

    if( !SO::EqualsNoCase(new_file_path, m_application.GetApplicationPropertiesFilePath()) &&
        PortableFunctions::FileIsRegular(new_file_path) )
    {
        try
        {
            // read the new application properties
            ApplicationProperties application_properties;
            application_properties.Open(new_file_path);

            const std::string message = FormatText("The file '%s' already exists. If you select this file, the current application properties will be "
                                                   "discarded in favor of the properties in this file. Are you sure you want to use this file?",
                                                   PortableFunctions::PathGetFilename(new_file_path).c_str());

            const int result = AfxMessageBox(message, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);

            if( result == IDYES )
            {
                // set them
                if( WindowsDesktopMessage::Send(UWM::CSPro::SetExternalApplicationProperties, &new_file_path, &application_properties) != 1 )
                    throw CSProException("There was an error setting the application properties.");

                // and then close the Application Properties dialog on success
                GetParent()->SendMessage(WM_CLOSE);
                return;
            }
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
        }

        new_file_path.clear();
    }

    m_applicationPropertiesFilePath = std::move(new_file_path);

    UpdateData(FALSE);
}
