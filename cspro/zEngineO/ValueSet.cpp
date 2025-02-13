#include "stdafx.h"
#include "ValueSet.h"
#include "ResponseProcessor.h"
#include <zToolsO/VectorHelpers.h>
#include <zUtilO/Randomizer.h>
#include <zDictO/ValueProcessor.h>
#include <zLogicO/ChildSymbolNames.h>
#include <engine/VarT.h>


// --------------------------------------------------------------------------
// ValueSet
// --------------------------------------------------------------------------

ValueSet::ValueSet(std::wstring value_set_name, VART* pVarT, const DictValueSet* dict_value_set, EngineData& engine_data)
    :   Symbol(std::move(value_set_name), SymbolType::ValueSet),
        m_engineData(engine_data),
        m_pVarT(pVarT),
        m_dictValueSet(dict_value_set)
{
}


ValueSet::ValueSet(const DictValueSet& dict_value_set, CSymbolVar* pVarT, EngineData& engine_data)
    :   ValueSet(CS2WS(dict_value_set.GetName()), pVarT, &dict_value_set, engine_data)
{
}


ValueSet::ValueSet(const ValueSet& value_set)
    :   Symbol(value_set),
        m_engineData(value_set.m_engineData),
        m_pVarT(nullptr),
        m_dictValueSet(nullptr)
{
    ASSERT(GetSubType() == SymbolSubType::DynamicValueSet);
}


ValueSet::~ValueSet()
{
}


bool ValueSet::IsNumeric() const
{
    return m_pVarT->IsNumeric();
}


const CString& ValueSet::GetLabel() const
{
    return m_dictValueSet->GetLabel();
}


size_t ValueSet::GetLength() const
{
    return GetResponseProcessor()->GetUnfilteredResponses().size();
}


std::shared_ptr<const ValueProcessor> ValueSet::GetSharedValueProcessor() const
{
    if( m_valueProcessor == nullptr )
        CreateValueProcessor();

    return m_valueProcessor;
}


const ValueProcessor& ValueSet::GetValueProcessor() const
{
    if( m_valueProcessor == nullptr )
        CreateValueProcessor();

    return *m_valueProcessor;
}


void ValueSet::CreateValueProcessor() const
{
    m_valueProcessor = ValueProcessor::CreateValueProcessor(*m_pVarT->GetDictItem(), &GetDictValueSet());
}


ResponseProcessor* ValueSet::GetResponseProcessor() const
{
    if( m_responseProcessor == nullptr )
        m_responseProcessor = std::make_unique<ResponseProcessor>(GetSharedValueProcessor());

    return m_responseProcessor.get();
}


void ValueSet::ForeachValue(const std::function<void(const ForeachValueInfo&, double, const std::optional<double>&)>& numeric_callback_function) const
{
    ASSERT(IsNumeric());

    for( const ValueSetResponse& response : VI_V(GetResponseProcessor()->GetUnfilteredResponses()) )
    {
        std::optional<double> to_value;

        if( !response.IsDiscrete() )
            to_value = response.GetMaximumValue();

        numeric_callback_function(ForeachValueInfo { response.GetLabel(), response.GetImageFilename(), response.GetTextColor() },
                                  response.GetMinimumValue(), to_value);
    }
}


void ValueSet::ForeachValue(const std::function<void(const ForeachValueInfo&, const CString&)>& string_callback_function) const
{
    ASSERT(IsString());

    for( const ValueSetResponse& response : VI_V(GetResponseProcessor()->GetUnfilteredResponses()) )
    {
        string_callback_function(ForeachValueInfo { response.GetLabel(), response.GetImageFilename(), response.GetTextColor() },
                                 response.GetCode());
    }
}


void ValueSet::Randomize(const std::vector<double>& numeric_exclusions, const std::vector<CString>& string_exclusions)
{
    const ValueProcessor& value_processor = GetValueProcessor();

    // first build up the list of exclusion values
    std::set<const DictValue*> values_to_exclude;

    if( IsNumeric() )
    {
        for( double exclusion_value : numeric_exclusions )
        {
            const DictValue* dict_value = value_processor.GetDictValue(exclusion_value);

            if( dict_value != nullptr )
                values_to_exclude.insert(dict_value);
        }
    }

    else
    {
        for( const CString& exclusion_value : string_exclusions )
        {
            const DictValue* dict_value = value_processor.GetDictValue(exclusion_value);

            if( dict_value != nullptr )
                values_to_exclude.insert(dict_value);
        }
    }

    GetResponseProcessor()->RandomizeResponses(values_to_exclude);
}


void ValueSet::Sort(bool ascending, bool sort_by_label)
{
    GetResponseProcessor()->SortResponses(ascending, sort_by_label);
}


Symbol* ValueSet::FindChildSymbol(const std::wstring& symbol_name) const
{
    constexpr const TCHAR* ListNames[] = { Logic::ValueSetCodes, Logic::ValueSetLabels };

    size_t list_index = 0;

    while( !SO::EqualsNoCase(symbol_name, ListNames[list_index]) )
    {
        if( ++list_index == _countof(ListNames) )
            return nullptr;
    }

    if( m_valueSetListWrapperIndices == nullptr )
        m_valueSetListWrapperIndices = std::make_unique<std::vector<int>>(_countof(ListNames), -1);

    int& value_set_list_wrapper_index = (*m_valueSetListWrapperIndices)[list_index];

    if( value_set_list_wrapper_index != -1 )
    {
        return &m_engineData.symbol_table.GetAt(value_set_list_wrapper_index);
    }

    else
    {
        // if the list doesn't exist yet, create one
        std::wstring value_set_list_wrapper_name = SO::Concatenate(GetName(), _T("."), ListNames[list_index]);
        
        std::shared_ptr<ValueSetListWrapper> new_wrapper_list(new ValueSetListWrapper(std::move(value_set_list_wrapper_name),
                                                                                      GetSymbolIndex(), ( list_index == 0 ), m_engineData));

        value_set_list_wrapper_index = m_engineData.AddSymbol(new_wrapper_list, Logic::SymbolTable::NameMapAddition::DoNotAdd);

        return new_wrapper_list.get();
    }
}


void ValueSet::serialize_subclass(Serializer& ar)
{
    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
        ar & m_valueSetListWrapperIndices;
}



// --------------------------------------------------------------------------
// DynamicValueSet
// --------------------------------------------------------------------------

struct DynamicValueSetEntry
{
    CString label;
    CString image_filename;
    PortableColor text_color;

    DynamicValueSetEntry(const CString& label_, const CString& image_filename_, PortableColor text_color_)
        :   label(label_),
            image_filename(image_filename_),
            text_color(std::move(text_color_))
    {
        label.TrimRight();
    }

    virtual ~DynamicValueSetEntry() { }
};


struct NumericDynamicValueSetEntry : public DynamicValueSetEntry
{
    double from_value;
    std::optional<double> to_value;

    NumericDynamicValueSetEntry(const CString& label_, const CString& image_filename_,
                                PortableColor text_color_, double from_value_, std::optional<double> to_value_)
        :   DynamicValueSetEntry(label_, image_filename_, std::move(text_color_)),
            from_value(from_value_),
            to_value(std::move(to_value_))
    {
    }
};


struct StringDynamicValueSetEntry : public DynamicValueSetEntry
{
    std::wstring value;

    StringDynamicValueSetEntry(const CString& label, const CString& image_filename_,
                               PortableColor text_color_, std::wstring value_)
        :   DynamicValueSetEntry(label, image_filename_, std::move(text_color_)),
            value(std::move(value_))
    {
        SO::MakeTrimRight(value);
    }
};


DynamicValueSet::DynamicValueSet(std::wstring value_set_name, EngineData& engine_data)
    :   ValueSet(std::move(value_set_name), nullptr, nullptr, engine_data),
        m_numeric(true)
{
    SetSubType(SymbolSubType::DynamicValueSet);
}


DynamicValueSet::DynamicValueSet(const DynamicValueSet& value_set)
    :   ValueSet(value_set),
        m_numeric(value_set.m_numeric)
{
}


std::unique_ptr<Symbol> DynamicValueSet::CloneInInitialState() const
{
    return std::unique_ptr<DynamicValueSet>(new DynamicValueSet(*this));
}


DynamicValueSet::~DynamicValueSet()
{
    DynamicValueSet::Reset();
}


size_t DynamicValueSet::GetLength() const
{
    return m_entries.size();
}


void DynamicValueSet::Reset()
{
    m_entries.clear();
}


void DynamicValueSet::ValidateNumericFromTo(double from_value, std::optional<double>& to_value) const
{
    // make sure an invalid special value isn't being added as the from value
    if( IsSpecial(from_value) && ( from_value == MISSING || from_value == REFUSED || from_value == DEFAULT ) )
    {
        throw CSProException(_T("You cannot add '%s' to the value set '%s' without specifying a code that it maps to"),
                                SpecialValues::ValueToString(from_value), GetName().c_str());
    }

    // range checks
    if( to_value.has_value() )
    {
        // clear the range if the from and to values are equal
        if( from_value == *to_value )
        {
            to_value.reset();
        }

        // the from value must be less than the to value
        else if( !IsSpecial(*to_value) && *to_value < from_value )
        {
            throw CSProException(_T("You cannot add a range to the value set '%s' where the from value is greater than the to value (%f > %f)"),
                                 GetName().c_str(), from_value, *to_value);
        }
    }
}


void DynamicValueSet::AddValue(std::wstring label, std::wstring image_filename, PortableColor text_color, double from_value, std::optional<double> to_value)
{
    ASSERT(IsNumeric());
    m_entries.emplace_back(std::make_unique<NumericDynamicValueSetEntry>(WS2CS(label), WS2CS(image_filename), std::move(text_color), from_value, std::move(to_value)));
}


void DynamicValueSet::AddValue(std::wstring label, std::wstring image_filename, PortableColor text_color, std::wstring value)
{
    ASSERT(IsString());
    m_entries.emplace_back(std::make_unique<StringDynamicValueSetEntry>(WS2CS(label), WS2CS(image_filename), std::move(text_color), std::move(value)));
}


size_t DynamicValueSet::AddValues(const ValueSet& value_set)
{
    size_t number_values_added = 0;

    if( m_numeric )
    {
        value_set.ForeachValue(
            [&](const ForeachValueInfo& info, double low_value, const std::optional<double>& high_value)
            {
                AddValue(CS2WS(info.label), CS2WS(info.image_filename), info.text_color, low_value, high_value);
                ++number_values_added;
            });
    }

    else
    {
        value_set.ForeachValue(
            [&](const ForeachValueInfo& info, const CString& value)
            {
                AddValue(CS2WS(info.label), CS2WS(info.image_filename), info.text_color, CS2WS(value));
                ++number_values_added;
            });
    }

    return number_values_added;
}


size_t DynamicValueSet::RemoveValue(double value)
{
    ASSERT(IsNumeric());
    size_t number_values_removed = 0;

    for( size_t i = m_entries.size() - 1; i < m_entries.size(); i-- )
    {
        const NumericDynamicValueSetEntry& numeric_entry = GetEntry<NumericDynamicValueSetEntry>(i);

        // remove based on the from value or based on the special value
        if( ( value == numeric_entry.from_value ) ||
            ( value == numeric_entry.to_value && IsSpecial(*numeric_entry.to_value) ) )
        {
            m_entries.erase(m_entries.begin() + i);
            ++number_values_removed;
        }
    }

    return number_values_removed;
}


size_t DynamicValueSet::RemoveValue(wstring_view value_sv)
{
    ASSERT(IsString());
    size_t number_values_removed = 0;

    value_sv = SO::TrimRight(value_sv);

    for( size_t i = m_entries.size() - 1; i < m_entries.size(); i-- )
    {
        const StringDynamicValueSetEntry& string_entry = GetEntry<StringDynamicValueSetEntry>(i);

        if( value_sv == string_entry.value )
        {
            m_entries.erase(m_entries.begin() + i);
            ++number_values_removed;
        }
    }

    return number_values_removed;
}


void DynamicValueSet::ForeachValue(const std::function<void(const ForeachValueInfo&, double, const std::optional<double>&)>& numeric_callback_function) const
{
    ASSERT(IsNumeric());

    for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
    {
        const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);

        numeric_callback_function(ForeachValueInfo { entry.label, entry.image_filename, entry.text_color },
                                  numeric_entry.from_value, numeric_entry.to_value);
    }
}


void DynamicValueSet::ForeachValue(const std::function<void(const ForeachValueInfo&, const CString&)>& string_callback_function) const
{
    ASSERT(IsString());

    for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
    {
        const StringDynamicValueSetEntry& string_entry = assert_cast<const StringDynamicValueSetEntry&>(entry);

        string_callback_function(ForeachValueInfo { entry.label, entry.image_filename, entry.text_color },
                                 WS2CS(string_entry.value));
    }
}


void DynamicValueSet::Randomize(const std::vector<double>& numeric_exclusions, const std::vector<CString>& string_exclusions)
{
    std::unique_ptr<std::vector<size_t>> indices_to_randomize;

    // get the filtered list of indices to randomize if necessary
    if( ( m_numeric && !numeric_exclusions.empty() ) || ( !m_numeric && !string_exclusions.empty() ) )
    {
        indices_to_randomize = std::make_unique<std::vector<size_t>>();

        for( size_t i = 0; i < m_entries.size(); ++i )
        {
            if( m_numeric )
            {
                const NumericDynamicValueSetEntry& numeric_entry = GetEntry<NumericDynamicValueSetEntry>(i);

                if( std::find(numeric_exclusions.cbegin(), numeric_exclusions.cend(), numeric_entry.from_value) != numeric_exclusions.cend() )
                    continue;
            }

            else
            {
                const StringDynamicValueSetEntry& string_entry = GetEntry<StringDynamicValueSetEntry>(i);

                if( std::find(string_exclusions.cbegin(), string_exclusions.cend(), WS2CS(string_entry.value)) != string_exclusions.cend() )
                    continue;
            }

            indices_to_randomize->emplace_back(i);
        }
    }

    std::default_random_engine random_engine(Randomizer::NextSeed());

    // if randomizing the whole value set, we can do this simply
    if( indices_to_randomize == nullptr || indices_to_randomize->size() == m_entries.size() )
    {
        std::shuffle(m_entries.begin(), m_entries.end(), std::move(random_engine));
    }

    else
    {
        VectorHelpers::Randomize(m_entries, std::move(*indices_to_randomize), std::move(random_engine));
    }
}


void DynamicValueSet::Sort(bool ascending, bool sort_by_label)
{
    std::sort(m_entries.begin(), m_entries.end(),
        [&](const std::unique_ptr<const DynamicValueSetEntry>& entry1, const std::unique_ptr<const DynamicValueSetEntry>& entry2)
        {
            double comparison;

            if( sort_by_label )
            {
                comparison = entry1->label.CompareNoCase(entry2->label);
            }

            else if( m_numeric )
            {
                const NumericDynamicValueSetEntry& numeric_entry1 = assert_cast<const NumericDynamicValueSetEntry&>(*entry1);
                const NumericDynamicValueSetEntry& numeric_entry2 = assert_cast<const NumericDynamicValueSetEntry&>(*entry2);
                comparison = numeric_entry1.from_value - numeric_entry2.from_value;
            }

            else
            {
                const StringDynamicValueSetEntry& string_entry1 = assert_cast<const StringDynamicValueSetEntry&>(*entry1);
                const StringDynamicValueSetEntry& string_entry2 = assert_cast<const StringDynamicValueSetEntry&>(*entry2);
                comparison = string_entry1.value.compare(string_entry2.value);
            }

            return ascending ? ( comparison < 0 ) :
                               ( comparison > 0 );
        });
}


void DynamicValueSet::serialize_subclass(Serializer& ar)
{
    ValueSet::serialize_subclass(ar);

    ar & m_numeric;
}



class ValueSetCreatedFromDynamicValueSet : public ValueSet
{
    // this class is simply ValueSet but with ownership of the dictionary value set memory
public:
    ValueSetCreatedFromDynamicValueSet(std::unique_ptr<const DictValueSet> dict_value_set, VART* pVarT, EngineData& engine_data)
        :   ValueSet(*dict_value_set, pVarT, engine_data),
            m_createdDictValueSet(std::move(dict_value_set))
    {
    }

private:
    std::unique_ptr<const DictValueSet> m_createdDictValueSet;
};


std::tuple<std::unique_ptr<DictValueSet>, bool> DynamicValueSet::CreateDictValueSet(size_t complete_length, size_t length, size_t decimals) const
{
    bool value_does_not_fit_in_value_set_warning = false;

    auto dict_value_set = std::make_unique<DictValueSet>();
    dict_value_set->SetName(WS2CS(GetName()));
    dict_value_set->SetLabel(WS2CS(GetName()));

    for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
    {
        DictValue dict_value;
        DictValuePair dict_value_pair;

        dict_value.SetLabel(entry.label);
        dict_value.SetImageFilename(entry.image_filename);
        dict_value.SetTextColor(entry.text_color);

        if( m_numeric )
        {
            const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);

            auto format_numeric_value = [&](double value)
            {
                std::wstring text_value = FormatTextCS2WS(_T("%*.*f"), (int)length, (int)decimals, value);
                value_does_not_fit_in_value_set_warning = value_does_not_fit_in_value_set_warning || ( text_value.length() > complete_length );
                SO::MakeTrim(text_value);
                return text_value;
            };

            if( numeric_entry.from_value == NOTAPPL )
            {
                dict_value_pair.SetFrom(WS2CS(std::wstring(length, ' ')));
                dict_value.SetSpecialValue(NOTAPPL);
            }

            else
            {
                dict_value_pair.SetFrom(WS2CS(format_numeric_value(numeric_entry.from_value)));
            }

            if( numeric_entry.to_value.has_value() )
            {
                if( IsSpecial(*numeric_entry.to_value) )
                {
                    dict_value.SetSpecialValue(*numeric_entry.to_value);
                }

                else
                {
                    dict_value_pair.SetTo(WS2CS(format_numeric_value(*numeric_entry.to_value)));
                }
            }
        }

        else
        {
            const StringDynamicValueSetEntry& string_entry = assert_cast<const StringDynamicValueSetEntry&>(entry);
            ASSERT(length == complete_length);

            value_does_not_fit_in_value_set_warning = value_does_not_fit_in_value_set_warning || ( string_entry.value.length() > length );

            std::wstring correct_length_value = string_entry.value;
            SO::MakeExactLength(correct_length_value, length);

            dict_value_pair.SetFrom(WS2CS(std::move(correct_length_value)));
        }

        dict_value.AddValuePair(std::move(dict_value_pair));
        dict_value_set->AddValue(std::move(dict_value));
    }

    return { std::move(dict_value_set), value_does_not_fit_in_value_set_warning };
}


std::unique_ptr<ValueSet> DynamicValueSet::CreateValueSet(VART* pVarT, bool& value_does_not_fit_in_value_set_warning) const
{
    const CDictItem* dict_item = pVarT->GetDictItem();
    ASSERT(dict_item != nullptr);

    std::unique_ptr<DictValueSet> dict_value_set;
    std::tie(dict_value_set, value_does_not_fit_in_value_set_warning) = CreateDictValueSet(dict_item->GetCompleteLen(), dict_item->GetLen(), dict_item->GetDecimal());

    return std::make_unique<ValueSetCreatedFromDynamicValueSet>(std::move(dict_value_set), pVarT, m_engineData);
}


namespace
{
    DictValue* CreateTemporaryDictValue(const DynamicValueSetEntry& entry, std::wstring value)
    {
        static DictValue dict_value;

        dict_value.SetLabel(entry.label);
        dict_value.SetImageFilename(entry.image_filename);
        dict_value.SetTextColor(entry.text_color);

        if( !dict_value.HasValuePairs() )
            dict_value.AddValuePair(DictValuePair());

        dict_value.GetValuePair(0).SetFrom(WS2CS(std::move(value)));

        return &dict_value;
    }

    const DynamicValueSetEntry* FindEntryByLabel(const std::vector<std::unique_ptr<const DynamicValueSetEntry>>& entries, wstring_view label_sv)
    {
        label_sv = SO::Trim(label_sv);

        for( const DynamicValueSetEntry& entry : VI_V(entries) )
        {
            if( SO::EqualsNoCase(label_sv, entry.label) )
                return &entry;
        }

        return nullptr;
    }
}


class NumericValueProcessorForDynamicValueSet : public NumericValueProcessor
{
public:
    NumericValueProcessorForDynamicValueSet(const std::vector<std::unique_ptr<const DynamicValueSetEntry>>& entries)
        :   m_entries(entries)
    {
    }

    double GetMinValue() const override
    {
        double min_value = DEFAULT;

        for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
        {
            const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);

            if( !IsSpecial(numeric_entry.from_value) )
            {
                if( numeric_entry.from_value < min_value || min_value == DEFAULT )
                    min_value = numeric_entry.from_value;
            }
        }

        return min_value;
    }

    double GetMaxValue() const override
    {
        double max_value = DEFAULT;

        for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
        {
            const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);

            double entry_max_value = numeric_entry.from_value;

            if( numeric_entry.to_value.has_value() && !IsSpecial(*numeric_entry.to_value) )
                entry_max_value = *numeric_entry.to_value;

            if( !IsSpecial(entry_max_value) )
            {
                if( entry_max_value > max_value || max_value == DEFAULT )
                    max_value = entry_max_value;
            }
        }

        return max_value;
    }

    bool IsValid(double value) const override
    {
        return ( FindEntryByValue(value) != nullptr );
    }

    const DictValue* GetDictValue(double value) const override
    {
        const NumericDynamicValueSetEntry* numeric_entry = FindEntryByValue(value);
        return ( numeric_entry != nullptr ) ? CreateTemporaryDictValueForNumeric(*numeric_entry) :
                                              nullptr;
    }

    const DictValue* GetDictValueByLabel(const CString& label) const override
    {
        const DynamicValueSetEntry* entry = FindEntryByLabel(m_entries, label);
        return ( entry != nullptr ) ? CreateTemporaryDictValueForNumeric(assert_cast<const NumericDynamicValueSetEntry&>(*entry)) :
                                      nullptr;
    }

private:
    const NumericDynamicValueSetEntry* FindEntryByValue(double value) const
    {
        for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
        {
            const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);
            bool matches = ( value == numeric_entry.from_value );

            if( !matches && numeric_entry.to_value.has_value() )
            {
                matches = ( value == *numeric_entry.to_value );

                if( !matches && !IsSpecial(value) )
                    matches = ( value >= numeric_entry.from_value && value <= *numeric_entry.to_value );
            }

            if( matches )
                return &numeric_entry;
        }

        return nullptr;
    }

    const DictValue* CreateTemporaryDictValueForNumeric(const NumericDynamicValueSetEntry& numeric_entry) const
    {
        // format the from code, right-trimming zeros
        CString from_value_text;

        if( !IsSpecial(numeric_entry.from_value) )
        {
            from_value_text.Format(_T("%0.6f"), numeric_entry.from_value);
            from_value_text.TrimRight(_T('0'));
            from_value_text.TrimRight(_T('.'));
        }

        return CreateTemporaryDictValue(numeric_entry, CS2WS(from_value_text));
    }

private:
    const std::vector<std::unique_ptr<const DynamicValueSetEntry>>& m_entries;
};


class StringValueProcessorForDynamicValueSet : public ValueProcessor
{
public:
    StringValueProcessorForDynamicValueSet(const std::vector<std::unique_ptr<const DynamicValueSetEntry>>& entries)
        :   m_entries(entries)
    {
    }

    bool IsValid(const CString& value, bool pad_value_to_length/* = true*/) const override
    {
        return ( FindEntryByValue(value, pad_value_to_length) != nullptr );
    }

    const DictValue* GetDictValue(const CString& value, bool pad_value_to_length/* = true*/) const override
    {
        const StringDynamicValueSetEntry* string_entry = FindEntryByValue(value, pad_value_to_length);
        return ( string_entry != nullptr ) ? CreateTemporaryDictValue(*string_entry, string_entry->value) :
                                             nullptr;
    }

    const DictValue* GetDictValueByLabel(const CString& label) const override
    {
        const DynamicValueSetEntry* entry = FindEntryByLabel(m_entries, label);
        return ( entry != nullptr ) ? CreateTemporaryDictValue(*entry, assert_cast<const StringDynamicValueSetEntry&>(*entry).value) :
                                      nullptr;
    }

private:
    const StringDynamicValueSetEntry* FindEntryByValue(wstring_view value_sv, bool pad_value_to_length) const
    {
        if( pad_value_to_length )
            value_sv = SO::TrimRight(value_sv);

        for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
        {
            const StringDynamicValueSetEntry& string_entry = assert_cast<const StringDynamicValueSetEntry&>(entry);

            if( SO::EqualsNoCase(value_sv, string_entry.value) )
                return &string_entry;
        }

        return nullptr;
    }

private:
    const std::vector<std::unique_ptr<const DynamicValueSetEntry>>& m_entries;
};


void DynamicValueSet::CreateValueProcessor() const
{
    if( m_numeric )
    {
        m_valueProcessor = std::make_shared<NumericValueProcessorForDynamicValueSet>(m_entries);
    }

    else
    {
        m_valueProcessor = std::make_shared<StringValueProcessorForDynamicValueSet>(m_entries);
    }
}



// --------------------------------------------------------------------------
// ValueSetListWrapper (list wrapper of the value set codes/labels)
// --------------------------------------------------------------------------

ValueSetListWrapper::ValueSetListWrapper(std::wstring value_set_list_wrapper_name, int value_set_symbol_index,
                                         bool codes_wrapper, const EngineData& engine_data)
    :   LogicList(std::move(value_set_list_wrapper_name)),
        m_engineData(engine_data),
        m_valueSetSymbolIndex(value_set_symbol_index),
        m_codesWrapper(codes_wrapper)
{
    SetSubType(SymbolSubType::ValueSetListWrapper);

    if( value_set_symbol_index != 0 )
        SetNumeric(m_codesWrapper && GetSymbolValueSet(m_valueSetSymbolIndex).IsNumeric());
}


ValueSetListWrapper::ValueSetListWrapper(std::wstring value_set_list_wrapper_name, const EngineData& engine_data)
    :   ValueSetListWrapper(std::move(value_set_list_wrapper_name), 0, false, engine_data)
{
}


void ValueSetListWrapper::serialize_subclass(Serializer& ar)
{
    LogicList::serialize_subclass(ar);

    ar & m_valueSetSymbolIndex
       & m_codesWrapper;
}


size_t ValueSetListWrapper::GetCount() const
{
    const ValueSet& value_set = GetSymbolValueSet(m_valueSetSymbolIndex);
    return value_set.GetLength();
}


double ValueSetListWrapper::GetValue(size_t index) const
{
    ASSERT(IsValidIndex(index) && m_codesWrapper);
    const ValueSet& value_set = GetSymbolValueSet(m_valueSetSymbolIndex);

    if( value_set.IsDynamic() )
    {
        const DynamicValueSet& dynamic_value_set = assert_cast<const DynamicValueSet&>(value_set);
        return dynamic_value_set.GetEntry<NumericDynamicValueSetEntry>(index - 1).from_value;
    }

    else
    {
        const std::vector<std::shared_ptr<const ValueSetResponse>>& responses = value_set.GetResponseProcessor()->GetUnfilteredResponses();
        return responses[index - 1]->GetMinimumValue();
    }
}


const std::wstring& ValueSetListWrapper::GetString(size_t index) const
{
    ASSERT(IsValidIndex(index));
    const ValueSet& value_set = GetSymbolValueSet(m_valueSetSymbolIndex);

    if( value_set.IsDynamic() )
    {
        const DynamicValueSet& dynamic_value_set = assert_cast<const DynamicValueSet&>(value_set);

        if( m_codesWrapper )
        {
            return dynamic_value_set.GetEntry<StringDynamicValueSetEntry>(index - 1).value;
        }

        else
        {
            return CS2WS_Reference(dynamic_value_set.m_entries[index - 1]->label);
        }
    }

    else
    {
        const std::vector<std::shared_ptr<const ValueSetResponse>>& responses = value_set.GetResponseProcessor()->GetUnfilteredResponses();

        if( m_codesWrapper )
        {
            return CS2WS_Reference(CString(responses[index - 1]->GetCode()).TrimRight());
        }

        else
        {
            return CS2WS_Reference(responses[index - 1]->GetLabel());
        }
    }
}


const Logic::SymbolTable& ValueSetListWrapper::GetSymbolTable() const
{
    return m_engineData.symbol_table;
}



// --------------------------------------------------------------------------
// JSON serialization
// --------------------------------------------------------------------------

void ValueSet::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    json_writer.Write(JK::label, GetLabel())
               .Write(JK::contentType, GetDataType());
}


void ValueSet::WriteValueToJson(JsonWriter& json_writer) const
{
    json_writer.Write(*m_dictValueSet);
}


void DynamicValueSet::WriteValueToJson(JsonWriter& json_writer) const
{
    // to make sure the CreateDictValueSet routine does not truncate any values, determine the minimum length necessary to hold the values
    size_t length = 1;
    size_t decimals = 0;

    if( m_numeric )
    {
        size_t integer_length = 1;

        for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
        {
            const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);

            auto process_number = [&](const double value)
            {
                if( !IsSpecial(value) )
                {
                    const std::wstring text_value = DoubleToString(value);

                    const size_t dot_pos = text_value.find('.');

                    if( dot_pos == std::wstring::npos )
                    {
                        integer_length = std::max(integer_length, text_value.length());
                    }

                    else
                    {
                        integer_length = std::max(integer_length, dot_pos);
                        decimals = std::max(decimals, text_value.length() - dot_pos - 1);
                    }

                }
            };

            process_number(numeric_entry.from_value);

            if( numeric_entry.to_value.has_value() )
                process_number(*numeric_entry.to_value);
        }

        // prioritize the integer length over the decimals
        decimals = std::min(decimals, static_cast<size_t>(MAX_DECIMALS));

        while( ( length = ( integer_length + decimals ) ) > static_cast<size_t>(MAX_NUMERIC_ITEM_LEN) )
        {
            if( decimals > 0 )
            {
                --decimals;
            }

            else
            {
                --integer_length;
            }
        }
    }

    else
    {
        for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
        {
            const StringDynamicValueSetEntry& string_entry = assert_cast<const StringDynamicValueSetEntry&>(entry);
            length = std::max(length, string_entry.value.length());
        }

        length = std::min(length, static_cast<size_t>(MAX_ALPHA_ITEM_LEN));
    }

    const size_t complete_length = length + ( ( decimals > 0 ) ? 1 : 0 );

    auto [dict_value_set, value_does_not_fit_in_value_set_warning] = CreateDictValueSet(complete_length, length, decimals);

    ASSERT(!value_does_not_fit_in_value_set_warning);

    json_writer.Write(*dict_value_set);
}


void DynamicValueSet::UpdateValueFromJson(const JsonNode<wchar_t>& json_node)
{
    // create entries from the value set
    std::vector<std::unique_ptr<const DynamicValueSetEntry>> new_entries;

    for( const JsonNode<wchar_t>& dict_value_node : json_node.GetArray(JK::values) )
    {
        DictValue dict_value = dict_value_node.Get<DictValue>();

        for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
        {
            if( m_numeric )
            {
                auto get_value = [](const CString& text_value)
                {
                    // the value may be special...
                    const double* special_value = SpecialValues::StringIsSpecial<const double*>(text_value);

                    if( special_value != nullptr )
                        return *special_value;

                    // ... or may be a real value
                    double value = atod(text_value);

                    if( value == IMSA_BAD_DOUBLE )
                        throw CSProException(_T("A numeric value set cannot store the code '%s'"), text_value.GetString());

                    return value;
                };

                const double from_value = get_value(dict_value_pair.GetFrom());

                // when no to value is specified, it can inherit the special value status from the DictValue,
                // which will be a special value when applicable, or std::nullopt if there is no special value (and thus no to value)
                std::optional<double> to_value = dict_value_pair.GetTo().IsEmpty() ? dict_value.GetSpecialValue<std::optional<double>>() :
                                                                                     std::make_optional(get_value(dict_value_pair.GetTo()));

                ValidateNumericFromTo(from_value, to_value);

                new_entries.emplace_back(std::make_unique<NumericDynamicValueSetEntry>(dict_value.GetLabel(), dict_value.GetImageFilename(),
                                                                                       dict_value.GetTextColor(), from_value, std::move(to_value)));
            }

            else
            {
                new_entries.emplace_back(std::make_unique<StringDynamicValueSetEntry>(dict_value.GetLabel(), dict_value.GetImageFilename(),
                                                                                      dict_value.GetTextColor(), CS2WS(dict_value_pair.GetFrom())));
            }
        }
    }

    m_entries = std::move(new_entries);
}


void ValueSetListWrapper::WriteValueToJson(JsonWriter& json_writer) const
{
    json_writer.BeginArray();

    const size_t count = GetCount();

    if( IsNumeric() )
    {
        for( size_t i = 1; i <= count; ++i )
            json_writer.WriteEngineValue(GetValue(i));
    }

    else
    {
        for( size_t i = 1; i <= count; ++i )
            json_writer.Write(GetString(i));
    }

    json_writer.EndArray();
}


void ValueSetListWrapper::UpdateValueFromJson(const JsonNode<wchar_t>& /*json_node*/)
{
    throw NoUpdateValueFromJsonRoutine("No JSON deserialization routine exists for the List objects in a ValueSet; deserialize the ValueSet instead");
}
