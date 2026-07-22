#include "stdafx.h"
#include "ValueSet.h"
#include "ResponseProcessor.h"
#include <engine/VarT.h>
#include <zToolsO/VectorHelpers.h>
#include <zUtilO/Randomizer.h>
#include <zDictO/ValueProcessor.h>
#include <zLogicO/ChildSymbolNames.h>
#include <zJavaScript/Executor.h>


// --------------------------------------------------------------------------
// ValueSet
// --------------------------------------------------------------------------

ValueSet::ValueSet(std::string value_set_name, VART* pVarT, const DictValueSet* const dict_value_set, EngineData& engine_data)
    :   Symbol(std::move(value_set_name), SymbolType::ValueSet),
        m_engineData(engine_data),
        m_pVarT(pVarT),
        m_dictValueSet(dict_value_set)
{
}


ValueSet::ValueSet(const DictValueSet& dict_value_set, CSymbolVar* const pVarT, EngineData& engine_data)
    :   ValueSet(dict_value_set.GetName(), pVarT, &dict_value_set, engine_data)
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
        const std::optional<double> to_value = !response.IsDiscrete()
            ? std::make_optional(response.GetMaximumValue())
            : std::nullopt;

        numeric_callback_function(
            ForeachValueInfo { response.GetLabelSharableString(), response.GetImageFilePath(), response.GetTextColor() },
            response.GetMinimumValue(),
            to_value
        );
    }
}


void ValueSet::ForeachValue(const std::function<void(const ForeachValueInfo&, const SharableString&)>& string_callback_function) const
{
    ASSERT(IsString());

    for( const ValueSetResponse& response : VI_V(GetResponseProcessor()->GetUnfilteredResponses()) )
    {
        string_callback_function(
            ForeachValueInfo { response.GetLabelSharableString(), response.GetImageFilePath(), response.GetTextColor() },
            response.GetCodeSharableString()
        );
    }
}


void ValueSet::Randomize(const std::variant<std::vector<double>, std::vector<SharableString>>& exclusions)
{
    const ValueProcessor& value_processor = GetValueProcessor();

    // first build up the list of exclusion values
    std::set<const DictValue*> values_to_exclude;

    if( IsNumeric() )
    {
        ASSERT(exclusions.index() == 0);

        for( const double exclusion : std::get<0>(exclusions) )
        {
            const DictValue* const dict_value = value_processor.GetDictValue(exclusion);

            if( dict_value != nullptr )
                values_to_exclude.insert(dict_value);
        }
    }

    else
    {
        ASSERT(exclusions.index() == 1);

        for( const SharableString& exclusion : std::get<1>(exclusions) )
        {
            const DictValue* const dict_value = value_processor.GetDictValue(*exclusion);

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


Symbol* ValueSet::FindChildSymbol(const std::string_view symbol_name_sv) const
{
    constexpr const char* ListNames[] = { Logic::ValueSetCodes, Logic::ValueSetLabels };

    size_t list_index = 0;

    while( !SO::EqualsNoCase(symbol_name_sv, ListNames[list_index]) )
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
        std::string value_set_list_wrapper_name = SO::Concatenate(GetName(), ".", ListNames[list_index]);

        std::shared_ptr<ValueSetListWrapper> new_wrapper_list(new ValueSetListWrapper(std::move(value_set_list_wrapper_name),
                                                                                      GetSymbolIndex(), ( list_index == 0 ), m_engineData));

        value_set_list_wrapper_index = m_engineData.AddSymbol(new_wrapper_list, Logic::SymbolTable::NameMapAddition::DoNotAdd);

        return new_wrapper_list.get();
    }
}


void ValueSet::CompareDeclarationAttributes(const Symbol& symbol) const
{
    const ValueSet& value_set = assert_cast<const ValueSet&>(symbol);

    if( GetDataType() != value_set.GetDataType() )
    {
        throw CompareDeclarationAttributesException("data type: %s vs. %s", ToString(GetDataType()),
                                                                            ToString(value_set.GetDataType()));
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
    SharableString label;
    std::string image_file_path;
    PortableColor text_color;

    DynamicValueSetEntry(SharableString label_, std::string image_file_path_, PortableColor text_color_)
        :   label(std::move(label_)),
            image_file_path(std::move(image_file_path_)),
            text_color(std::move(text_color_))
    {
        label.MakeTrimRight();
    }

    virtual ~DynamicValueSetEntry() { }
};


struct NumericDynamicValueSetEntry : public DynamicValueSetEntry
{
    double from_value;
    std::optional<double> to_value;

    NumericDynamicValueSetEntry(SharableString label_, std::string image_file_path_,
                                PortableColor text_color_, double from_value_, std::optional<double> to_value_)
        :   DynamicValueSetEntry(std::move(label_), std::move(image_file_path_), std::move(text_color_)),
            from_value(from_value_),
            to_value(std::move(to_value_))
    {
    }

    bool CodeIsEqual(const NumericDynamicValueSetEntry& rhs) const
    {
        return ( from_value == rhs.from_value &&
                 to_value == rhs.to_value );
    }
};


struct StringDynamicValueSetEntry : public DynamicValueSetEntry
{
    SharableString value;

    StringDynamicValueSetEntry(SharableString label_, std::string image_file_path_,
                               PortableColor text_color_, SharableString value_)
        :   DynamicValueSetEntry(std::move(label_), std::move(image_file_path_), std::move(text_color_)),
            value(std::move(value_))
    {
        value.MakeTrimRight();
    }

    bool CodeIsEqual(const StringDynamicValueSetEntry& rhs) const
    {
        return ( value == rhs.value );
    }
};


DynamicValueSet::DynamicValueSet(std::string value_set_name, EngineData& engine_data)
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
        throw CSProException("You cannot add '%s' to the value set '%s' without specifying a code that it maps to",
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
            throw CSProException("You cannot add a range to the value set '%s' where the from value is greater than the to value (%f > %f)",
                                 GetName().c_str(), from_value, *to_value);
        }
    }
}


void DynamicValueSet::AddValue(SharableString label, std::string image_file_path, PortableColor text_color, const double from_value, std::optional<double> to_value)
{
    ASSERT(IsNumeric());

    m_entries.emplace_back(std::make_unique<NumericDynamicValueSetEntry>(
        std::move(label),
        std::move(image_file_path),
        std::move(text_color),
        from_value,
        std::move(to_value)
    ));
}


void DynamicValueSet::AddValue(SharableString label, std::string image_file_path, PortableColor text_color, SharableString value)
{
    ASSERT(IsString());

    m_entries.emplace_back(std::make_unique<StringDynamicValueSetEntry>(
        std::move(label),
        std::move(image_file_path),
        std::move(text_color),
        std::move(value)
    ));
}


size_t DynamicValueSet::AddValues(const ValueSet& value_set)
{
    size_t number_values_added = 0;

    if( m_numeric )
    {
        value_set.ForeachValue(
            [&](const ForeachValueInfo& info, const double low_value, const std::optional<double>& high_value)
            {
                AddValue(
                    info.label,
                    info.image_file_path,
                    info.text_color,
                    low_value,
                    high_value
                );

                ++number_values_added;
            });
    }

    else
    {
        value_set.ForeachValue(
            [&](const ForeachValueInfo& info, const SharableString& value)
            {
                AddValue(
                    info.label,
                    info.image_file_path,
                    info.text_color,
                    value
                );

                ++number_values_added;
            });
    }

    return number_values_added;
}


size_t DynamicValueSet::RemoveValue(const double value)
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


size_t DynamicValueSet::RemoveValue(std::string_view value_sv)
{
    ASSERT(IsString());
    size_t number_values_removed = 0;

    value_sv = SO::TrimRight(value_sv);

    for( size_t i = m_entries.size() - 1; i < m_entries.size(); i-- )
    {
        const StringDynamicValueSetEntry& string_entry = GetEntry<StringDynamicValueSetEntry>(i);

        if( value_sv == *string_entry.value )
        {
            m_entries.erase(m_entries.begin() + i);
            ++number_values_removed;
        }
    }

    return number_values_removed;
}


size_t DynamicValueSet::RemoveDuplicates(const RemoveDuplicatesType remove_type)
{
    if( m_entries.size() < 2 )
        return 0;

    return m_numeric ? RemoveDuplicatesWorker<NumericDynamicValueSetEntry>(remove_type) :
                       RemoveDuplicatesWorker<StringDynamicValueSetEntry>(remove_type);
}


template<typename EntryT>
size_t DynamicValueSet::RemoveDuplicatesWorker(const RemoveDuplicatesType remove_type)
{
    ASSERT(m_entries.size() >= 2);

    size_t duplicates_removed = 0;

    for( auto itr = m_entries.end() - 1; itr > m_entries.begin(); --itr )
    {
        const auto& lookup = std::find_if(m_entries.begin(), itr,
            [&](const auto& comparison)
            {
                const EntryT& entry1 = assert_cast<const EntryT&>(*(*itr));
                const EntryT& entry2 = assert_cast<const EntryT&>(*comparison);

                switch( remove_type )
                {
                    case RemoveDuplicatesType::ByCodeLabel:
                        return ( entry1.CodeIsEqual(entry2) &&
                                 entry1.label == entry2.label );

                    case RemoveDuplicatesType::ByCode:
                        return entry1.CodeIsEqual(entry2);

                    case RemoveDuplicatesType::ByLabel:
                        return ( entry1.label == entry2.label );

                    default:
                        return ReturnProgrammingError(false);
                }
            });

        if( lookup < itr )
        {
            itr = m_entries.erase(itr);
            ++duplicates_removed;
        }
    }

    return duplicates_removed;
}


void DynamicValueSet::ForeachValue(const std::function<void(const ForeachValueInfo&, double, const std::optional<double>&)>& numeric_callback_function) const
{
    ASSERT(IsNumeric());

    for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
    {
        const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);

        numeric_callback_function(
            ForeachValueInfo { entry.label, entry.image_file_path, entry.text_color },
            numeric_entry.from_value,
            numeric_entry.to_value
        );
    }
}


void DynamicValueSet::ForeachValue(const std::function<void(const ForeachValueInfo&, const SharableString&)>& string_callback_function) const
{
    ASSERT(IsString());

    for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
    {
        const StringDynamicValueSetEntry& string_entry = assert_cast<const StringDynamicValueSetEntry&>(entry);

        string_callback_function(
            ForeachValueInfo { entry.label, entry.image_file_path, entry.text_color },
            string_entry.value
        );
    }
}


void DynamicValueSet::Randomize(const std::variant<std::vector<double>, std::vector<SharableString>>& exclusions)
{
    std::optional<std::vector<size_t>> indices_to_randomize;

    // get the filtered list of indices to randomize if necessary
    ASSERT(exclusions.index() == ( m_numeric ? 0 : 1 ));

    if( !( m_numeric ? std::get<0>(exclusions).empty() : std::get<1>(exclusions).empty() ) )
    {
        indices_to_randomize.emplace();

        for( size_t i = 0; i < m_entries.size(); ++i )
        {
            if( m_numeric )
            {
                const NumericDynamicValueSetEntry& numeric_entry = GetEntry<NumericDynamicValueSetEntry>(i);
                const std::vector<double>& numeric_exclusions = std::get<0>(exclusions);

                if( std::find(numeric_exclusions.cbegin(), numeric_exclusions.cend(), numeric_entry.from_value) != numeric_exclusions.cend() )
                    continue;
            }

            else
            {
                const StringDynamicValueSetEntry& string_entry = GetEntry<StringDynamicValueSetEntry>(i);
                const std::vector<SharableString>& string_exclusions = std::get<1>(exclusions);

                if( std::find(string_exclusions.cbegin(), string_exclusions.cend(), string_entry.value) != string_exclusions.cend() )
                    continue;
            }

            indices_to_randomize->emplace_back(i);
        }
    }

    std::default_random_engine random_engine(Randomizer::NextSeed());

    // if randomizing the whole value set, we can do this simply
    if( !indices_to_randomize.has_value() || indices_to_randomize->size() == m_entries.size() )
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
                comparison = SO::CompareNoCase(*entry1->label, *entry2->label);
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
                comparison = string_entry1.value->compare(*string_entry2.value);
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
    dict_value_set->SetName(GetName());
    dict_value_set->SetLabel(UTF8_TODO::GetCString(GetName()));

    for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
    {
        DictValue dict_value;
        DictValuePair dict_value_pair;

        dict_value.SetLabel(UTF8_TODO::GetCString(entry.label));
        dict_value.SetImageFilePath(entry.image_file_path);
        dict_value.SetTextColor(entry.text_color);

        if( m_numeric )
        {
            const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);

            auto format_numeric_value = [&](const double value)
            {
                std::string text_value = FormatText("%*.*f", static_cast<int>(length), static_cast<int>(decimals), value);
                value_does_not_fit_in_value_set_warning = ( value_does_not_fit_in_value_set_warning ||
                                                            SO::WideLength(text_value) > complete_length );
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
                dict_value_pair.SetFrom(UTF8_TODO::GetCString(format_numeric_value(numeric_entry.from_value)));
            }

            if( numeric_entry.to_value.has_value() )
            {
                if( IsSpecial(*numeric_entry.to_value) )
                {
                    dict_value.SetSpecialValue(*numeric_entry.to_value);
                }

                else
                {
                    dict_value_pair.SetTo(UTF8_TODO::GetCString(format_numeric_value(*numeric_entry.to_value)));
                }
            }
        }

        else
        {
            const StringDynamicValueSetEntry& string_entry = assert_cast<const StringDynamicValueSetEntry&>(entry);
            ASSERT(length == complete_length);

            value_does_not_fit_in_value_set_warning = value_does_not_fit_in_value_set_warning || ( string_entry.value->length() > length );

            std::string correct_length_value = *string_entry.value;
            SO::WideMakeExactLength(correct_length_value, length);

            dict_value_pair.SetFrom(UTF8_TODO::GetCString(std::move(correct_length_value)));
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

        dict_value.SetLabel(UTF8_TODO::GetCString(entry.label));
        dict_value.SetImageFilePath(entry.image_file_path);
        dict_value.SetTextColor(entry.text_color);

        if( !dict_value.HasValuePairs() )
            dict_value.AddValuePair(DictValuePair());

        dict_value.GetValuePair(0).SetFrom(WS2CS(std::move(value)));

        return &dict_value;
    }

    const DynamicValueSetEntry* FindEntryByLabel(const std::vector<std::unique_ptr<const DynamicValueSetEntry>>& entries, std::string_view label_sv)
    {
        label_sv = SO::Trim(label_sv);

        for( const DynamicValueSetEntry& entry : VI_V(entries) )
        {
            if( SO::EqualsNoCase(label_sv, *entry.label) )
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
        std::optional<double> min_value;

        for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
        {
            const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);

            // special values are not counted as min/max values
            if( numeric_entry.to_value.has_value() && IsSpecial(*numeric_entry.to_value) )
                continue;

            if( !min_value.has_value() || numeric_entry.from_value < *min_value )
                min_value = numeric_entry.from_value;
        }

        return min_value.value_or(DEFAULT);
    }

    double GetMaxValue() const override
    {
        std::optional<double> max_value;

        for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
        {
            const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);
            double entry_max_value;

            if( numeric_entry.to_value.has_value() )
            {
                // special values are not counted as min/max values
                if( IsSpecial(*numeric_entry.to_value) )
                    continue;

                entry_max_value = *numeric_entry.to_value;
            }

            else
            {
                entry_max_value = numeric_entry.from_value;
            }

            if( !max_value.has_value() || entry_max_value > *max_value)
                max_value = entry_max_value;
        }

        return max_value.value_or(DEFAULT);
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

    const DictValue* GetDictValueByLabel(const std::string_view label_sv) const override
    {
        const DynamicValueSetEntry* const entry = FindEntryByLabel(m_entries, label_sv);
        return ( entry != nullptr ) ? CreateTemporaryDictValueForNumeric(assert_cast<const NumericDynamicValueSetEntry&>(*entry)) :
                                      nullptr;
    }

private:
    const NumericDynamicValueSetEntry* FindEntryByValue(double value) const
    {
        for( const DynamicValueSetEntry& entry : VI_V(m_entries) )
        {
            const NumericDynamicValueSetEntry& numeric_entry = assert_cast<const NumericDynamicValueSetEntry&>(entry);

            if( !numeric_entry.to_value.has_value() ? ( value == numeric_entry.from_value ) :
                IsSpecial(*numeric_entry.to_value)  ? ( value == *numeric_entry.to_value ) :
                                                      ( value >= numeric_entry.from_value && value <= *numeric_entry.to_value ) )
            {
                return &numeric_entry;
            }
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
        return ( string_entry != nullptr ) ? CreateTemporaryDictValue(*string_entry, UTF8_TODO::GetWide(*string_entry->value)) :
                                             nullptr;
    }

    const DictValue* GetDictValueByLabel(const std::string_view label_sv) const override
    {
        const DynamicValueSetEntry* const entry = FindEntryByLabel(m_entries, label_sv);
        return ( entry != nullptr ) ? CreateTemporaryDictValue(*entry, UTF8_TODO::GetWide(*assert_cast<const StringDynamicValueSetEntry&>(*entry).value)) :
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

            if( SO::EqualsNoCase(value_sv, UTF8_TODO::GetWide(*string_entry.value)) )
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

ValueSetListWrapper::ValueSetListWrapper(std::string value_set_list_wrapper_name, int value_set_symbol_index,
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


ValueSetListWrapper::ValueSetListWrapper(std::string value_set_list_wrapper_name, const EngineData& engine_data)
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


double ValueSetListWrapper::GetValueNumeric(const size_t index) const
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


const SharableString& ValueSetListWrapper::GetValueString(const size_t index) const
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
            return dynamic_value_set.m_entries[index - 1]->label;
        }
    }

    else
    {
        const std::vector<std::shared_ptr<const ValueSetResponse>>& responses = value_set.GetResponseProcessor()->GetUnfilteredResponses();

        if( m_codesWrapper )
        {
            ASSERT(SO::TrimRight(responses[index - 1]->GetCode()).length() == responses[index - 1]->GetCode().length());
            return responses[index - 1]->GetCodeSharableString();
        }

        else
        {
            return responses[index - 1]->GetLabelSharableString();
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
                    const std::wstring text_value = UTF8_TODO::GetWide(DoubleToString(value));

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
            length = std::max(length, SO::WideLength(*string_entry.value));
        }

        length = std::min(length, static_cast<size_t>(MAX_ALPHA_ITEM_LEN));
    }

    const size_t complete_length = length + ( ( decimals > 0 ) ? 1 : 0 );

    auto [dict_value_set, value_does_not_fit_in_value_set_warning] = CreateDictValueSet(complete_length, length, decimals);

    ASSERT(!value_does_not_fit_in_value_set_warning);

    json_writer.Write(*dict_value_set);
}


void DynamicValueSet::SetValueFromJson(const JsonNode& json_node)
{
    // create entries from the value set
    std::vector<std::unique_ptr<const DynamicValueSetEntry>> new_entries;

    for( const JsonNode& dict_value_node : json_node.GetArray(JK::values) )
    {
        DictValue dict_value = dict_value_node.Get<DictValue>();

        for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
        {
            if( m_numeric )
            {
                auto get_value = [](const CString& text_value)
                {
                    // the value may be special...
                    const double* const special_value = SpecialValues::StringIsSpecial<const double*>(UTF8_TODO::GetUtf8(text_value));

                    if( special_value != nullptr )
                        return *special_value;

                    // ... or may be a real value
                    double value = atod(text_value);

                    if( value == IMSA_BAD_DOUBLE )
                        throw CSProException("A numeric value set cannot store the code '%s'", UTF8_TODO::GetUtf8(text_value).c_str());

                    return value;
                };

                const double from_value = get_value(dict_value_pair.GetFrom());

                // when no to value is specified, it can inherit the special value status from the DictValue,
                // which will be a special value when applicable, or std::nullopt if there is no special value (and thus no to value)
                std::optional<double> to_value = dict_value_pair.GetTo().IsEmpty() ? dict_value.GetSpecialValue<std::optional<double>>() :
                                                                                     std::make_optional(get_value(dict_value_pair.GetTo()));

                ValidateNumericFromTo(from_value, to_value);

                new_entries.emplace_back(std::make_unique<NumericDynamicValueSetEntry>(
                    UTF8_TODO::GetUtf8(dict_value.GetLabel()),
                    dict_value.GetImageFilePath(),
                    dict_value.GetTextColor(),
                    from_value,
                    std::move(to_value)
                ));
            }

            else
            {
                new_entries.emplace_back(std::make_unique<StringDynamicValueSetEntry>(
                    UTF8_TODO::GetUtf8(dict_value.GetLabel()),
                    dict_value.GetImageFilePath(),
                    dict_value.GetTextColor(),
                    UTF8_TODO::GetUtf8(dict_value_pair.GetFrom())
                ));
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
            json_writer.WriteEngineValue(GetValueNumeric(i));
    }

    else
    {
        for( size_t i = 1; i <= count; ++i )
            json_writer.Write(GetValueString(i));
    }

    json_writer.EndArray();
}


void ValueSetListWrapper::SetValueFromJson(const JsonNode& /*json_node*/)
{
    throw NoSetValueFromJsonRoutine("No JSON deserialization routine exists for the List objects in a ValueSet; deserialize the ValueSet instead");
}



// --------------------------------------------------------------------------
// JavaScript serialization
// --------------------------------------------------------------------------

JavaScript::Value ValueSetListWrapper::GetJavaScriptValue(JavaScript::Executor& executor) const
{
    const size_t count = GetCount();

    auto js_array_values = std::make_unique_for_overwrite<JavaScript::Value[]>(count);
    JavaScript::Value* js_array_values_itr = js_array_values.get();

    for( size_t i = 1; i <= count; ++i )
    {
        new (js_array_values_itr) JavaScript::Value(IsNumeric() ? executor.CreateEngineValue(GetValueNumeric(i)) :
                                                                  executor.CreateEngineValue(GetValueString(i)));
        ++js_array_values_itr;
    }

    return executor.CreateArray(count, js_array_values.get());
}


void ValueSetListWrapper::SetValueFromJavaScript(JavaScript::Executor& /*executor*/, const JavaScript::Value& /*js_value*/)
{
    throw CSProException("The List '%s' is read-only and cannot be modified", GetName().c_str());
}
