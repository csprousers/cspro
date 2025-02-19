#include "StdAfx.h"
#include "CapiStyle.h"


// Most common fonts - should be available on all platforms
const std::vector<const wchar_t*> CapiStyle::DefaultFontNames = { L"Arial", L"Courier New", L"Times New Roman" };
const std::vector<int> CapiStyle::DefaultFontSizes = { 8, 9, 10, 11, 12, 14, 16, 18, 24, 36 };


void CapiStyle::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::name, name)
               .Write(JK::className, class_name)
               .Write(JK::css, css)
               .EndObject();
}


void CapiStyle::serialize(Serializer& ar)
{
    ar & name
       & class_name
       & css;
}
