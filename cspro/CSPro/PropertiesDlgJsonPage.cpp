#include "StdAfx.h"
#include "PropertiesDlgJsonPage.h"


PropertiesDlgJsonPage::PropertiesDlgJsonPage(JsonProperties& json_properties, CWnd* pParent/* = nullptr*/)
    :   CDialog(PropertiesDlgJsonPage::IDD, pParent),
        m_jsonProperties(json_properties),
        m_jsonFormatRadioEnumHelper({ JsonProperties::JsonFormat::Compact,
                                      JsonProperties::JsonFormat::Pretty }),
        m_arrayFormatRadioEnumHelper({ JsonProperties::ArrayFormat::Full,
                                       JsonProperties::ArrayFormat::Sparse }),
        m_hashMapFormatRadioEnumHelper({ JsonProperties::HashMapFormat::Array,
                                         JsonProperties::HashMapFormat::Object }),
        m_binaryDataFormatRadioEnumHelper({ JsonProperties::BinaryDataFormat::DataUrl,
                                           JsonProperties::BinaryDataFormat::LocalhostUrl })
{
    PropertiesToForm(json_properties);
}


void PropertiesDlgJsonPage::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_CBIndex(pDX, IDC_JSON_FORMAT, m_jsonFormat);
    DDX_CBIndex(pDX, IDC_JSON_ARRAY, m_arrayFormat);
    DDX_CBIndex(pDX, IDC_JSON_HASHMAP, m_hashMapFormat);
    DDX_CBIndex(pDX, IDC_JSON_BINARY_CONTENT, m_binaryDataFormat);
}


void PropertiesDlgJsonPage::PropertiesToForm(const JsonProperties& json_properties)
{
    m_jsonFormat = m_jsonFormatRadioEnumHelper.ToForm(json_properties.GetJsonFormat());
    m_arrayFormat = m_arrayFormatRadioEnumHelper.ToForm(json_properties.GetArrayFormat());
    m_hashMapFormat = m_hashMapFormatRadioEnumHelper.ToForm(json_properties.GetHashMapFormat());
    m_binaryDataFormat = m_binaryDataFormatRadioEnumHelper.ToForm(json_properties.GetBinaryDataFormat());
}


void PropertiesDlgJsonPage::FormToProperties()
{
    UpdateData(TRUE);

    m_jsonProperties.SetJsonFormat(m_jsonFormatRadioEnumHelper.FromForm(m_jsonFormat));
    m_jsonProperties.SetArrayFormat(m_arrayFormatRadioEnumHelper.FromForm(m_arrayFormat));
    m_jsonProperties.SetHashMapFormat(m_hashMapFormatRadioEnumHelper.FromForm(m_hashMapFormat));
    m_jsonProperties.SetBinaryDataFormat(m_binaryDataFormatRadioEnumHelper.FromForm(m_binaryDataFormat));
}


void PropertiesDlgJsonPage::ResetProperties()
{
    JsonProperties default_json_properties;

    PropertiesToForm(default_json_properties);

    UpdateData(FALSE);
}


void PropertiesDlgJsonPage::OnOK()
{
    FormToProperties();

    CDialog::OnOK();
}
