#include "StdAfx.h"
#include "CodeFilePropertiesPageDlg.h"


unsigned int CodeFilePropertiesPageDlg::GetDialogTemplateId()
{
    return IDD_PAGE_PROPERTIES_CODE_FILE;
}


CodeFilePropertiesPageDlg::CodeFilePropertiesPageDlg(CodeFile code_file, CWnd* const pParent/* = nullptr*/)
    :   CDialog(GetDialogTemplateId(), pParent),
        m_codeFile(std::move(code_file)),
        m_codeTypeRadioEnumHelper({ CodeType::LogicMain,
                                    CodeType::LogicExternal,
                                    CodeType::JavaScriptAutodetect,
                                    CodeType::JavaScriptGlobal,
                                    CodeType::JavaScriptModule }),
        m_codeType(m_codeTypeRadioEnumHelper.ToForm(m_codeFile.GetCodeType()))
{
}


void CodeFilePropertiesPageDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_CODE_CSPRO_LOGIC_MAIN, m_codeType);
}


BOOL CodeFilePropertiesPageDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    // the main logic file's code type cannot be modified
    if( m_codeFile.IsLogicMain() )
    {
        GetDlgItem(IDC_CODE_CSPRO_LOGIC_EXTERNAL)->EnableWindow(FALSE);
        GetDlgItem(IDC_CODE_JAVASCRIPT_AUTODETECT)->EnableWindow(FALSE);
        GetDlgItem(IDC_CODE_JAVASCRIPT_GLOBAL)->EnableWindow(FALSE);
        GetDlgItem(IDC_CODE_JAVASCRIPT_MODULE)->EnableWindow(FALSE);
    }

    else
    {
        GetDlgItem(IDC_CODE_CSPRO_LOGIC_MAIN)->EnableWindow(FALSE);
    }

    return result;
}


void CodeFilePropertiesPageDlg::OnOK()
{
    UpdateData(TRUE);

    m_codeFile.SetCodeType(m_codeTypeRadioEnumHelper.FromForm(m_codeType));
}
