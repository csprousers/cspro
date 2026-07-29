#pragma once

#include <zDictO/zDictO.h>

class CDictItem;
class DictValue;
class DictValueSet;
class ValueSetResponse;


// --------------------------------------------------------------------------
// ValueProcessor
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTO ValueProcessor
{
protected:
    ValueProcessor(const CDictItem* dict_item, const DictValueSet* dict_value_set) noexcept;

public:
    virtual ~ValueProcessor() noexcept { }

    // Creates a ValueProcessor subclass based on the item and whether or not it has a value set.
    static std::shared_ptr<const ValueProcessor> CreateValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set = nullptr);

    const CDictItem* GetDictItem() const noexcept        { return m_dictItem; }
    const DictValueSet* GetDictValueSet() const noexcept { return m_dictValueSet; }

    // Returns whether the value is valid.
    virtual bool IsValid(double value) const = 0;
    virtual bool IsValid(std::string_view value_sv, bool pad_value_to_length = true) const = 0;

    // Returns the first dictionary value that contains the value, or null if no match.
    virtual const DictValue* GetDictValue(double value) const = 0;
    virtual const DictValue* GetDictValue(std::string_view value_sv, bool pad_value_to_length = true) const = 0;

    // Returns the first dictionary value that contains the label, or null if no match.
    virtual const DictValue* GetDictValueByLabel(std::string_view label_sv) const;

    // Returns all dictionary values that contain the value.
    virtual std::vector<const DictValue*> GetMatchingDictValues(double value) const = 0;
    virtual std::vector<const DictValue*> GetMatchingDictValues(std::string_view value_sv) const = 0;

    // Parses the string input value, returning its representation based on item or value set characteristics.
    // For example, "-99" might return MISSING due to a mapping in the value set,
    // or "Hello" might return "He" based on an item's length.
    virtual double GetNumericFromInput(std::string_view value_sv) const = 0;
    virtual std::string GetAlphaFromInput(std::string value) const = 0;

    // Returns the dictionary value associated with the string input value.
    virtual const DictValue* GetDictValueFromInput(std::string_view value_sv) const = 0;

    // Converts the value to its string representation for output.
    // For example, MISSING might return "-99" due to a mapping in the value set,
    // or "Hello" might return "Hello    " based on an item's length.
    virtual std::string GetOutput(double value) const = 0;
    virtual std::string GetOutput(std::string value) const = 0;

    // Returns a vector of ValueSetResponse objects (wrapping the dictionary's values).
    virtual const std::vector<std::shared_ptr<const ValueSetResponse>>& GetResponses() const = 0;

protected:
    const CDictItem* m_dictItem;
    const DictValueSet* m_dictValueSet;
};
