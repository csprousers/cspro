#pragma once

#include <zCapiO/zCapiO.h>


struct CLASS_DECL_ZCAPIO CapiStyle
{
    std::string name;
    std::string class_name;
    std::string css;

    static const std::vector<const wchar_t*> DefaultFontNames;
    static const std::vector<int> DefaultFontSizes;


    // serialization
    // --------------------------------------------------
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);
};
