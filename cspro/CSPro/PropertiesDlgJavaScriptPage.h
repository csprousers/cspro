#pragma once

#include <CSPro/PropertiesDlgPage.h>


class PropertiesDlgJavaScriptPage : public CDialog, public PropertiesDlgPage
{
public:
    enum { IDD = IDD_PROPERTIES_JAVASCRIPT };

    PropertiesDlgJavaScriptPage(JavaScriptProperties& javascript_properties, CWnd* pParent = nullptr);

    void FormToProperties() override;
    void ResetProperties() override;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnUseActionInvokerChange();

private:
    void PropertiesToForm(const JavaScriptProperties& javascript_properties);

    void EnableDisableControls();

private:
    JavaScriptProperties& m_javaScriptProperties;

    BOOL m_abortOnModuleLoadError;

    BOOL m_useActionInvoker;
    std::string m_actionInvokerObjectNameOverride;

    int m_bytecodeSerialization;
    RadioEnumHelper<JavaScriptProperties::BytecodeSerialization> m_bytecodeSerializationRadioEnumHelper;
};
