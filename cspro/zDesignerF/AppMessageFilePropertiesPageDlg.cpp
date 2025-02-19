#include "StdAfx.h"
#include "AppMessageFilePropertiesPageDlg.h"


unsigned int AppMessageFilePropertiesPageDlg::GetDialogTemplateId()
{
    return IDD_PAGE_PROPERTIES_APP_MESSAGE_FILE;
}


AppMessageFilePropertiesPageDlg::AppMessageFilePropertiesPageDlg(AppMessageFile app_message_file, const bool main_message_file, CWnd* const pParent/* = nullptr*/)
    :   CDialog(GetDialogTemplateId(), pParent),
        m_appMessageFile(std::move(app_message_file)),
        m_mainMessageFile(main_message_file),
        m_typeRadioEnumHelper({ AppMessageFile::Type::User,
                                AppMessageFile::Type::System }),
        m_type(m_typeRadioEnumHelper.ToForm(m_appMessageFile.GetType()))
{
}


void AppMessageFilePropertiesPageDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_MESSAGES_USER, m_type);
}


BOOL AppMessageFilePropertiesPageDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    // the main message file cannot be system messages
    if( m_mainMessageFile )
    {
        ASSERT(m_type == 0);
        GetDlgItem(IDC_MESSAGES_SYSTEM)->EnableWindow(FALSE);
    }

    return result;
}


void AppMessageFilePropertiesPageDlg::OnOK()
{
    UpdateData(TRUE);

    m_appMessageFile.SetType(m_typeRadioEnumHelper.FromForm(m_type));
}
