#pragma once

#include <zCapiO/zCapiO.h>

class JsonWriter;
class Serializer;


struct CLASS_DECL_ZCAPIO CapiStyle
{
    CString m_name;
    CString m_class_name;
    CString m_css;

    static const std::vector<CString> DefaultFontNames;
    static const std::vector<int> DefaultFontSizes;


    // serialization
    // --------------------------------------------------
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);
};
