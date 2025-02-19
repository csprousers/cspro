#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/DDClass.h>


class CLASS_DECL_ZDICTO ValueSetResponse
{
public:
    ValueSetResponse(const CDictItem& dict_item, const DictValue& dict_value, const DictValuePair& dict_value_pair);
    ValueSetResponse(const CString& label, double value);

    const CString& GetLabel() const             { return ( m_dictValue != nullptr ) ? m_dictValue->GetLabel() : std::get<0>(*m_nonDictValues); }
    const CString& GetCode() const              { return m_code; }
    const std::string& GetImageFilePath() const { return ( m_dictValue != nullptr ) ? m_dictValue->GetImageFilePath() : std::get<1>(*m_nonDictValues); }
    const PortableColor& GetTextColor() const   { return ( m_dictValue != nullptr ) ? m_dictValue->GetTextColor() : std::get<2>(*m_nonDictValues); }

    double GetMinimumValue() const { return m_minValue; }
    bool IsDiscrete() const        { return !m_maxValue.has_value(); }
    double GetMaximumValue() const;

    const DictValue* GetDictValue() const { return m_dictValue; }

    static std::string FormatValueForDisplay(const CDictItem& dict_item, double value);

private:
    const DictValue* m_dictValue;
    CString m_code;
    double m_minValue;
    std::optional<double> m_maxValue;

    std::unique_ptr<std::tuple<CString, std::string, PortableColor>> m_nonDictValues; // label, image file path, color
};
