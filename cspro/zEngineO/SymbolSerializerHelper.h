#pragma once

#include <zToolsO/SerializerHelper.h>
#include <zAppO/Properties/ApplicationProperties.h>

class BinarySymbol;


class SymbolSerializerHelper : public SerializerHelper::Helper
{
public:
    SymbolSerializerHelper(const JsonProperties& json_properties)
        :   m_jsonProperties(json_properties)
    {
    }

    const JsonProperties& GetJsonProperties() const { return m_jsonProperties; }

    virtual std::wstring LocalhostCreateMappingForBinarySymbol(const BinarySymbol& /*binary_symbol*/) { return ReturnProgrammingError(std::wstring()); }

private:
    const JsonProperties& m_jsonProperties;
};
