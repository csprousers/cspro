#include "StdAfx.h"
#include "CapiStyle.h"


// Most common fonts - should be available on all platforms
const std::vector<CString> CapiStyle::DefaultFontNames = { L"Arial", L"Courier New", L"Times New Roman" };
const std::vector<int> CapiStyle::DefaultFontSizes = { 8, 9, 10, 11, 12, 14, 16, 18, 24, 36 };


void CapiStyle::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::name, m_name)
               .Write(JK::className, m_class_name)
               .Write(JK::css, m_css)
               .EndObject();
}


void CapiStyle::serialize(Serializer& ar)
{
    ar & m_name
       & m_class_name
       & m_css;
}
