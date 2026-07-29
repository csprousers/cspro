#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/DDClass.h>


class CLASS_DECL_ZDICTO ValueSetResponse
{
public:
    ValueSetResponse(const CDictItem& dict_item, const DictValue& dict_value, const DictValuePair& dict_value_pair);
    ValueSetResponse(SharableString label, double value);

    const std::string& GetLabel() const noexcept { return *GetLabelSharableString(); }
    const SharableString& GetLabelSharableString() const noexcept;

    const std::string& GetCode() const noexcept                  { return *m_code; }
    const SharableString& GetCodeSharableString() const noexcept { return m_code; }

    const std::string& GetImageFilePath() const noexcept;

    const PortableColor& GetTextColor() const noexcept;

    double GetMinimumValue() const noexcept { return m_minValue; }
    bool IsDiscrete() const noexcept        { return !m_maxValue.has_value(); }
    double GetMaximumValue() const;

    const DictValue* GetDictValue() const noexcept { return m_dictValue; }

    static std::string FormatValueForDisplay(const CDictItem& dict_item, double value);

private:
    const DictValue* m_dictValue;
    SharableString m_code;
    double m_minValue;
    std::optional<double> m_maxValue;

    struct NDV { SharableString label; std::string image_file_path; PortableColor text_color; };
    std::unique_ptr<NDV> m_nonDictValues;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline const SharableString& ValueSetResponse::GetLabelSharableString() const noexcept
{
    return ( m_dictValue != nullptr ) ? UTF8_TODO::Create_SharableStringReference(m_dictValue->GetLabel())
                                      : m_nonDictValues->label;
}


inline const std::string& ValueSetResponse::GetImageFilePath() const noexcept
{
    return ( m_dictValue != nullptr ) ? m_dictValue->GetImageFilePath()
                                      : m_nonDictValues->image_file_path;
}


inline const PortableColor& ValueSetResponse::GetTextColor() const noexcept
{
    return ( m_dictValue != nullptr ) ? m_dictValue->GetTextColor()
                                      : m_nonDictValues->text_color;
}
