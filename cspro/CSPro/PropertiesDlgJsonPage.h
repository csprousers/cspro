#pragma once

#include <CSPro/PropertiesDlgPage.h>


class PropertiesDlgJsonPage : public CDialog, public PropertiesDlgPage
{
public:
    enum { IDD = IDD_PROPERTIES_JSON };

    PropertiesDlgJsonPage(JsonProperties& json_properties, CWnd* pParent = nullptr);

    void FormToProperties() override;
    void ResetProperties() override;

protected:
    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

private:
    void PropertiesToForm(const JsonProperties& json_properties);

private:
    JsonProperties& m_jsonProperties;

    int m_jsonFormat;
    RadioEnumHelper<JsonProperties::JsonFormat> m_jsonFormatRadioEnumHelper;

    int m_arrayFormat;
    RadioEnumHelper<JsonProperties::ArrayFormat> m_arrayFormatRadioEnumHelper;

    int m_hashMapFormat;
    RadioEnumHelper<JsonProperties::HashMapFormat> m_hashMapFormatRadioEnumHelper;

    int m_binaryDataFormat;
    RadioEnumHelper<JsonProperties::BinaryDataFormat> m_binaryDataFormatRadioEnumHelper;
};
