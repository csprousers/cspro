#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/DictBase.h>
#include <zDictO/DictValuePair.h>
#include <zUtilO/PortableColor.h>


class CLASS_DECL_ZDICTO DictValue : public DictBase
{
public:
    DictValue();

    DictElementType GetElementType() const override { return DictElementType::Value; }

    const CString& GetImageFilename() const             { return m_imageFilename; }
    void SetImageFilename(const CString&image_filename) { m_imageFilename = image_filename; }

    const PortableColor& GetTextColor() const          { return m_textColor; }
    void SetTextColor(const PortableColor& text_color) { m_textColor = text_color; }

    bool IsSpecial() const { return m_specialValue.has_value(); }
    void SetNotSpecial()   { m_specialValue.reset(); }

    template<typename T = double>
    T GetSpecialValue() const;

    bool IsSpecialValue(double special_value) const { return ( IsSpecial() && GetSpecialValue() == special_value ); }
    void SetSpecialValue(double value);
    void SetSpecialValue(const std::optional<double>& value);

    size_t GetNumValuePairs() const { return m_dictValuePairs.size(); }
    bool HasValuePairs() const      { return !m_dictValuePairs.empty(); }

    const std::vector<DictValuePair>& GetValuePairs() const { return m_dictValuePairs; }
    std::vector<DictValuePair>& GetValuePairs()             { return m_dictValuePairs; }

    const DictValuePair& GetValuePair(size_t index) const   { ASSERT(index < m_dictValuePairs.size()); return m_dictValuePairs[index]; }
    DictValuePair& GetValuePair(size_t index)               { ASSERT(index < m_dictValuePairs.size()); return m_dictValuePairs[index]; }

    void AddValuePair(DictValuePair dict_value_pair);
    void InsertValuePair(size_t index, DictValuePair dict_value_pair);
    void RemoveValuePair(size_t index);

    // returns the number of value pairs that have defined "to" values
    size_t GetNumToValues() const;

    // returns a range string that can be used with proportions
    CString GetRangeString() const;


    // serialization
    static DictValue CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    CString m_imageFilename;
    PortableColor m_textColor;
    std::optional<double> m_specialValue;
    std::vector<DictValuePair> m_dictValuePairs;
};



template<typename T>
T DictValue::GetSpecialValue() const
{
    if constexpr(std::is_same_v<T, double>)
    {
        ASSERT(IsSpecial());
        return *m_specialValue;
    }

    else
    {
        return m_specialValue;
    }
}
