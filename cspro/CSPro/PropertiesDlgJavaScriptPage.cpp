#include "StdAfx.h"
#include "PropertiesDlgJavaScriptPage.h"


BEGIN_MESSAGE_MAP(PropertiesDlgJavaScriptPage, CDialog)
    ON_BN_CLICKED(IDC_JAVASCRIPT_USE_ACTION_INVOKER, OnUseActionInvokerChange)
END_MESSAGE_MAP()


PropertiesDlgJavaScriptPage::PropertiesDlgJavaScriptPage(JavaScriptProperties& javascript_properties, CWnd* const pParent/* = nullptr*/)
    :   CDialog(PropertiesDlgJavaScriptPage::IDD, pParent),
        m_javaScriptProperties(javascript_properties),
        m_bytecodeSerializationRadioEnumHelper({ JavaScriptProperties::BytecodeSerialization::ScriptAndBytecode,
                                                 JavaScriptProperties::BytecodeSerialization::ScriptOnly,
                                                 JavaScriptProperties::BytecodeSerialization::BytecodeOnly })
{
    PropertiesToForm(javascript_properties);
}


void PropertiesDlgJavaScriptPage::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_JAVASCRIPT_ABORT_MODULE_LOAD_ERROR, m_abortOnModuleLoadError);
    DDX_Check(pDX, IDC_JAVASCRIPT_USE_ACTION_INVOKER, m_useActionInvoker);
    DDX_Text(pDX, IDC_JAVASCRIPT_ACTION_INVOKER_NAME, m_actionInvokerObjectNameOverride);
    DDX_Radio(pDX, IDC_JAVASCRIPT_SCRIPTS_AND_BYTECODE, m_bytecodeSerialization);
}


BOOL PropertiesDlgJavaScriptPage::OnInitDialog()
{
    CDialog::OnInitDialog();

    EnableDisableControls();

    return TRUE;
}


void PropertiesDlgJavaScriptPage::PropertiesToForm(const JavaScriptProperties& javascript_properties)
{
    m_abortOnModuleLoadError = javascript_properties.GetAbortOnModuleLoadError();
    m_useActionInvoker = javascript_properties.GetUseActionInvoker();
    m_actionInvokerObjectNameOverride = javascript_properties.GetEvaluatedActionInvokerObjectName();
    m_bytecodeSerialization = m_bytecodeSerializationRadioEnumHelper.ToForm(javascript_properties.GetBytecodeSerialization());
}


void PropertiesDlgJavaScriptPage::FormToProperties()
{
    UpdateData(TRUE);

    if( m_useActionInvoker && m_actionInvokerObjectNameOverride.empty() )
        throw CSProException("You must specify the Action Invoker name.");

    m_javaScriptProperties.SetAbortOnModuleLoadError(m_abortOnModuleLoadError);
    m_javaScriptProperties.SetUseActionInvoker(m_useActionInvoker);
    m_javaScriptProperties.SetActionInvokerObjectNameOverride(m_actionInvokerObjectNameOverride);
    m_javaScriptProperties.SetBytecodeSerialization(m_bytecodeSerializationRadioEnumHelper.FromForm(m_bytecodeSerialization));
}


void PropertiesDlgJavaScriptPage::ResetProperties()
{
    JavaScriptProperties default_javascript_properties;

    PropertiesToForm(default_javascript_properties);

    UpdateData(FALSE);
    EnableDisableControls();
}


void PropertiesDlgJavaScriptPage::OnOK()
{
    FormToProperties();

    CDialog::OnOK();
}


void PropertiesDlgJavaScriptPage::EnableDisableControls()
{
    GetDlgItem(IDC_JAVASCRIPT_ACTION_INVOKER_NAME)->EnableWindow(m_useActionInvoker);
}


void PropertiesDlgJavaScriptPage::OnUseActionInvokerChange()
{
    UpdateData(TRUE);
    EnableDisableControls();
}
