#include "StdAfx.h"
#include "DictionaryComparer.h"


bool DictionaryDifference::IsDestructive() const
{
    switch( type )
    {
        case Type::LevelRemoved:
        case Type::RecordRemoved:
        case Type::RecordOccurrencesDecreased:
        case Type::ItemRemoved:
        case Type::ItemValueTruncated:
        case Type::ItemContentTypeChangedInvalidSometimes:
        case Type::ItemContentTypeChangedInvalidAlways:
        case Type::ItemMovedToDifferentRecordOccurrencesDecreased:
        case Type::ItemItemSubitemOccurrencesDeceased:
            return true;
    }

    return false;
}


const DictNamedBase* DictionaryDifference::GetDefinedDictElement() const
{
    return ( initial_dict_element != nullptr ) ? initial_dict_element :
                                                 final_dict_element;
}


bool DictionaryDifference::IsLevelDifference() const
{
    const DictNamedBase* dict_element = GetDefinedDictElement();

    return ( dict_element != nullptr &&
             dict_element->GetElementType() == DictElementType::Level );
}


bool DictionaryDifference::IsRecordDifference() const
{
    const DictNamedBase* dict_element = GetDefinedDictElement();

    return ( dict_element != nullptr &&
             dict_element->GetElementType() == DictElementType::Record );
}


bool DictionaryDifference::IsItemDifference() const
{
    const DictNamedBase* dict_element = GetDefinedDictElement();

    return ( dict_element != nullptr &&
             dict_element->GetElementType() == DictElementType::Item );
}


std::wstring DictionaryDifference::GetDisplayName(const DictNamedBase* dict_element,
                                                  const bool show_names/* = true*/,
                                                  const size_t display_name_length/* = SIZE_MAX*/)
{
    if( dict_element == nullptr )
        return std::wstring();

    std::wstring display_name = CS2WS(show_names ? dict_element->GetName() :
                                                   dict_element->GetLabel());

    if( display_name.length() > display_name_length )
    {
        constexpr wstring_view Ellipsis_sv = _T("...");

        if( display_name_length < Ellipsis_sv.length() )
            return Ellipsis_sv.substr(0, display_name_length);

        display_name.resize(display_name_length - Ellipsis_sv.length());
        display_name.append(Ellipsis_sv);
    }

    return display_name;
}


std::wstring DictionaryDifference::GetDisplayName(const bool show_names/* = true*/, const size_t display_name_length/* = SIZE_MAX*/) const
{
    return GetDisplayName(GetDefinedDictElement(), show_names, display_name_length);
}



DictionaryComparer::DictionaryComparer(const CDataDict& initial_dictionary, const CDataDict& final_dictionary)
    :   m_initialDictionary(initial_dictionary),
        m_finalDictionary(final_dictionary)
{
    CompareDictionary();
}


void DictionaryComparer::CompareDictionary()
{
    // match the first level regardless of name and then levels after that based on name;
    // when there is a mismatch, ignore all subsequent levels
    const size_t number_levels = std::max(m_initialDictionary.GetNumLevels(), m_finalDictionary.GetNumLevels());
    bool process_levels = true;

    for( size_t level_number = 0; level_number < number_levels; ++level_number )
    {
        const DictLevel* initial_dict_level = ( level_number < m_initialDictionary.GetNumLevels() ) ? &m_initialDictionary.GetLevel(level_number) : nullptr;
        const DictLevel* final_dict_level = ( level_number < m_finalDictionary.GetNumLevels() ) ? &m_finalDictionary.GetLevel(level_number) : nullptr;

        if( level_number == 0 || ( process_levels && initial_dict_level != nullptr && final_dict_level != nullptr &&
                                   SO::EqualsNoCase(initial_dict_level->GetName(), final_dict_level->GetName()) ) )
        {
            CompareLevel(*initial_dict_level, *final_dict_level);
        }

        else
        {
            if( initial_dict_level != nullptr )
                AddDifference(DictionaryDifference::Type::LevelRemoved, initial_dict_level, nullptr);

            if( final_dict_level != nullptr )
                AddDifference(DictionaryDifference::Type::LevelAdded, nullptr, final_dict_level);

            process_levels = false;
        }            
    }
}


void DictionaryComparer::CompareLevel(const DictLevel& initial_dict_level, const DictLevel& final_dict_level)
{
    // compare the IDs
    CompareRecord(initial_dict_level.GetIdItemsRec(), final_dict_level.GetIdItemsRec());

    // compare the records
    auto find_matching_record_by_name = [](const DictLevel& in_dict_level, const std::wstring& record_name) -> const CDictRecord*
    {
        for( int record = 0; record < in_dict_level.GetNumRecords(); ++record )
        {
            const CDictRecord* in_dict_record = in_dict_level.GetRecord(record);

            if( SO::EqualsNoCase(in_dict_record->GetName(), record_name) )
                return in_dict_record;
        }

        return nullptr;
    };

    for( int record = 0; record < initial_dict_level.GetNumRecords(); ++record )
    {
        const CDictRecord* initial_dict_record = initial_dict_level.GetRecord(record);
        const CDictRecord* final_dict_record = find_matching_record_by_name(final_dict_level, CS2WS(initial_dict_record->GetName()));

        if( final_dict_record != nullptr )
        {
            CompareRecord(initial_dict_record, final_dict_record);
        }

        else
        {
            AddDifference(DictionaryDifference::Type::RecordRemoved, initial_dict_record, nullptr);
        }
    }

    for( int record = 0; record < final_dict_level.GetNumRecords(); ++record )
    {
        const CDictRecord* final_dict_record = final_dict_level.GetRecord(record);
        const CDictRecord* initial_dict_record = find_matching_record_by_name(initial_dict_level, CS2WS(final_dict_record->GetName()));

        if( initial_dict_record == nullptr )
        {
            if( final_dict_record->GetRequired() )
            {
                AddDifference(DictionaryDifference::Type::RequiredRecordAdded, nullptr, final_dict_record);
            }

            else
            {
                AddDifference(DictionaryDifference::Type::NonRequiredRecordAdded, nullptr, final_dict_record);
            }
        }
    }


    // compare items (across records but on the same level)

    // first generate the list of all of the items
    std::map<std::wstring, ItemPair> item_pairs;

    for( int level_set = 0; level_set < 2; ++level_set )
    {
        const bool use_initial = ( level_set == 0 );

        auto add_items_from_record = [&](const CDictRecord* dict_record, const bool processing_ids)
        {
            for( int item = 0; item < dict_record->GetNumItems(); ++item )
            {
                const CDictItem* dict_item = dict_record->GetItem(item);
                std::wstring item_name = SO::ToUpper(dict_item->GetName());

                auto item_pair_lookup = item_pairs.find(item_name);

                if( item_pair_lookup == item_pairs.end() )
                    item_pair_lookup = item_pairs.try_emplace(std::move(item_name), ItemPair { nullptr, nullptr, false }).first;

                ItemPair& item_pair = item_pair_lookup->second;

                if( use_initial )
                {
                    item_pair.initial_dict_item = dict_item;
                }

                else
                {
                    item_pair.final_dict_item = dict_item;
                }

                item_pair.item_is_an_id |= processing_ids;
            }
        };

        const DictLevel& dict_level = use_initial ? initial_dict_level : final_dict_level;

        add_items_from_record(dict_level.GetIdItemsRec(), true);

        for( int record = 0; record < dict_level.GetNumRecords(); ++record )
            add_items_from_record(dict_level.GetRecord(record), false);
    }

    // now process the items
    for( const auto& [item_name, item_pair]  : item_pairs )
    {
        if( item_pair.initial_dict_item == nullptr )
        {
            AddDifference(DictionaryDifference::Type::ItemAdded, item_pair);
        }

        else if( item_pair.final_dict_item == nullptr )
        {
            AddDifference(DictionaryDifference::Type::ItemRemoved, item_pair);
        }

        else
        {
            CompareItem(item_pair);
            m_matchedItemPairs.emplace_back(item_pair);
        }
    }
}


void DictionaryComparer::CompareRecord(const CDictRecord* initial_dict_record, const CDictRecord* final_dict_record)
{
    if( initial_dict_record->GetRecTypeVal().Compare(final_dict_record->GetRecTypeVal()) != 0 )
        AddDifference(DictionaryDifference::Type::RecordTypeChanged, initial_dict_record, final_dict_record);

    if( initial_dict_record->GetMaxRecs() > final_dict_record->GetMaxRecs() )
    {
        AddDifference(DictionaryDifference::Type::RecordOccurrencesDecreased, initial_dict_record, final_dict_record);
    }

    else if( initial_dict_record->GetMaxRecs() < final_dict_record->GetMaxRecs() )
    {
        AddDifference(DictionaryDifference::Type::RecordOccurrencesIncreased, initial_dict_record, final_dict_record);
    }

    if( !initial_dict_record->GetRequired() && final_dict_record->GetRequired() )
        AddDifference(DictionaryDifference::Type::RequiredRecordOccurrenceAdded, initial_dict_record, final_dict_record);
}


void DictionaryComparer::CompareItem(const ItemPair& item_pair)
{
    // check the length
    if( item_pair.initial_dict_item->GetLen() != item_pair.final_dict_item->GetLen() )
    {
        AddDifference(DictionaryDifference::Type::ItemLengthChanged, item_pair);

        bool value_truncated = ( item_pair.initial_dict_item->GetLen() > item_pair.final_dict_item->GetLen() );

        // the value can be truncated if the numeric integer or decimal portions are reduced
        if( !value_truncated && IsNumeric(*item_pair.initial_dict_item) && IsNumeric(*item_pair.final_dict_item) )
        {
            value_truncated = ( item_pair.initial_dict_item->GetIntegerLen() > item_pair.final_dict_item->GetIntegerLen() ) ||
                              ( item_pair.initial_dict_item->GetDecimal() > item_pair.final_dict_item->GetDecimal() );
        }

        if( value_truncated )
            AddDifference(DictionaryDifference::Type::ItemValueTruncated, item_pair);
    }


    // check the content type
    if( item_pair.initial_dict_item->GetContentType() != item_pair.final_dict_item->GetContentType() )
    {
        const std::optional<DictionaryDifference::Type> content_type_difference =
            GetItemContentTypeDifference(item_pair.initial_dict_item->GetContentType(), item_pair.final_dict_item->GetContentType());

        if( content_type_difference.has_value() )
            AddDifference(*content_type_difference, item_pair);
    }


    // check the formatting for numeric values
    if( IsNumeric(*item_pair.initial_dict_item) && IsNumeric(*item_pair.final_dict_item) )
    {
        if( ( item_pair.initial_dict_item->GetZeroFill() != item_pair.final_dict_item->GetZeroFill() ) ||
            ( item_pair.initial_dict_item->GetDecimal() > 0 && item_pair.final_dict_item->GetDecimal() > 0 && item_pair.initial_dict_item->GetDecChar() != item_pair.final_dict_item->GetDecChar() ) )
        {
            AddDifference(DictionaryDifference::Type::ItemFormattingChanged, item_pair);
        }
    }


    // check if the item has moved to a different record
    const CDictRecord* initial_dict_record = item_pair.initial_dict_item->GetRecord();
    const CDictRecord* final_dict_record = item_pair.final_dict_item->GetRecord();

    if( initial_dict_record->GetName().CompareNoCase(final_dict_record->GetName()) != 0 )
    {
        AddDifference(DictionaryDifference::Type::ItemMovedToDifferentRecord, item_pair);

        // check if the number of record occurrences has decreased
        if( initial_dict_record->GetMaxRecs() > final_dict_record->GetMaxRecs() )
            AddDifference(DictionaryDifference::Type::ItemMovedToDifferentRecordOccurrencesDecreased, item_pair);
    }

    // or somewhere else on the same different record
    else if( item_pair.initial_dict_item->GetStart() != item_pair.final_dict_item->GetStart() )
    {
        AddDifference(DictionaryDifference::Type::ItemMovedOnRecord, item_pair);
    }


    // check if the number of item/subitem occurrences has changed
    const int item_subitem_occurrences_difference = static_cast<int>(item_pair.initial_dict_item->GetItemSubitemOccurs()) -
                                                    static_cast<int>(item_pair.final_dict_item->GetItemSubitemOccurs());

    if( item_subitem_occurrences_difference > 0 )
    {
        AddDifference(DictionaryDifference::Type::ItemItemSubitemOccurrencesDeceased, item_pair);
    }

    else if( item_subitem_occurrences_difference < 0 )
    {
        AddDifference(DictionaryDifference::Type::ItemItemSubitemOccurrencesIncreased, item_pair);
    }
}


std::optional<DictionaryDifference::Type> DictionaryComparer::GetItemContentTypeDifference(const ContentType initial_content_type, const ContentType final_content_type)
{
    // binary checks...
    if( IsBinary(initial_content_type) || IsBinary(final_content_type) )
    {
        // binary data cannot be converted to non-binary data and vice versa
        if( !IsBinary(initial_content_type) || !IsBinary(final_content_type) )
        {
            return DictionaryDifference::Type::ItemContentTypeChangedInvalidAlways;
        }

        // all binary data can be converted to a document
        else if( final_content_type == ContentType::Document )
        {
            return DictionaryDifference::Type::ItemContentTypeChangedValidConversionUnnecessary;
        }

        // all documents can potentially be converted to a non-document type
        else if( initial_content_type == ContentType::Document )
        {
            return DictionaryDifference::Type::ItemContentTypeChangedInvalidSometimes;
        }

        // non-document data cannot be converted to non-document types
        else
        {
            return DictionaryDifference::Type::ItemContentTypeChangedInvalidAlways;
        }
    }

    // numeric data can always be converted to string data
    else if( IsNumeric(initial_content_type) && IsString(final_content_type) )
    {
        return DictionaryDifference::Type::ItemContentTypeChangedValidConversionNeeded;
    }

    // string data can potentially be converted to numeric data
    else if( IsString(initial_content_type) && IsNumeric(final_content_type) )
    {
        return DictionaryDifference::Type::ItemContentTypeChangedInvalidSometimes;
    }

    else
    {
        ASSERT(false);
        return std::nullopt;
    }
}


void DictionaryComparer::AddDifference(const DictionaryDifference::Type type, const DictNamedBase* initial_dict_element, const DictNamedBase* final_dict_element)
{
    m_differences.emplace_back(DictionaryDifference { type, initial_dict_element, final_dict_element, false });
}


void DictionaryComparer::AddDifference(const DictionaryDifference::Type type, const ItemPair& item_pair)
{
    m_differences.emplace_back(DictionaryDifference { type, item_pair.initial_dict_item, item_pair.final_dict_item, item_pair.item_is_an_id });
}


bool DictionaryComparer::RequiresReformat(const std::vector<DataStorageCharacteristic>* data_storage_characteristics) const
{
    auto check_data_storage_characteristic = [&](DataStorageCharacteristic characteristic)
    {
        return ( data_storage_characteristics != nullptr && std::find(data_storage_characteristics->cbegin(),
                                                                      data_storage_characteristics->cend(), characteristic) != data_storage_characteristics->cend() );
    };

    for( const DictionaryDifference& difference : m_differences )
    {
        bool requires_reformat = false;

        switch( difference.type )
        {
            case DictionaryDifference::Type::LevelRemoved:
            case DictionaryDifference::Type::RecordRemoved:
            case DictionaryDifference::Type::RequiredRecordAdded:
            case DictionaryDifference::Type::RecordOccurrencesDecreased:
            case DictionaryDifference::Type::RequiredRecordOccurrenceAdded:
            case DictionaryDifference::Type::ItemRemoved:
            case DictionaryDifference::Type::ItemValueTruncated:
            case DictionaryDifference::Type::ItemContentTypeChangedValidConversionNeeded:
            case DictionaryDifference::Type::ItemContentTypeChangedInvalidSometimes:
            case DictionaryDifference::Type::ItemContentTypeChangedInvalidAlways:
            case DictionaryDifference::Type::ItemMovedToDifferentRecord:
            case DictionaryDifference::Type::ItemMovedToDifferentRecordOccurrencesDecreased:
            case DictionaryDifference::Type::ItemItemSubitemOccurrencesDeceased:
            case DictionaryDifference::Type::ItemItemSubitemOccurrencesIncreased:
                requires_reformat = true;
                break;

            case DictionaryDifference::Type::RecordTypeChanged:
                requires_reformat = check_data_storage_characteristic(DataStorageCharacteristic::UsesRecordType);
                break;

            case DictionaryDifference::Type::ItemLengthChanged:
            case DictionaryDifference::Type::ItemMovedOnRecord:
                requires_reformat = check_data_storage_characteristic(DataStorageCharacteristic::UsesFixedWidthAndPositions);
                break;

            // some changes don't necessarily require a reformat:
            // case DictionaryDifference::Type::LevelAdded:
            // case DictionaryDifference::Type::NonRequiredRecordAdded:
            // case DictionaryDifference::Type::RecordOccurrencesIncreased:
            // case DictionaryDifference::Type::ItemAdded:
            // case DictionaryDifference::Type::ItemContentTypeChangedValidConversionUnnecessary:
            // case DictionaryDifference::Type::ItemFormattingChanged:
        }

        // some optional changes are required when applied to IDs
        if( !requires_reformat && difference.key_related )
        {
            switch( difference.type )
            {
                case DictionaryDifference::Type::ItemAdded:
                case DictionaryDifference::Type::ItemLengthChanged:
                case DictionaryDifference::Type::ItemFormattingChanged:
                    requires_reformat = true;
                    break;
            }
        }

        if( requires_reformat )
            return true;
    }

    return false;
}


std::vector<DictionaryDifference> DictionaryComparer::GetDataRepositorySpecificDifferences(const DataRepositoryType data_repository_type)
{
    // this is only used now when comparing embedded dictionaries 
    ASSERT(data_repository_type == DataRepositoryType::SQLite || data_repository_type == DataRepositoryType::EncryptedSQLite);

    std::vector<DictionaryDifference> differences;

    for( const DictionaryDifference& difference : m_differences )
    {
        bool include_difference = false;

        switch( difference.type )
        {
            case DictionaryDifference::Type::RecordOccurrencesDecreased:
            case DictionaryDifference::Type::ItemRemoved:
            case DictionaryDifference::Type::ItemMovedToDifferentRecord:
            case DictionaryDifference::Type::ItemItemSubitemOccurrencesDeceased:
            {
                include_difference = true;
                break;
            }


            // item content type changes
            case DictionaryDifference::Type::ItemContentTypeChangedInvalidAlways:
            {
                include_difference = true;
                break;
            }

            case DictionaryDifference::Type::ItemContentTypeChangedValidConversionNeeded:
            case DictionaryDifference::Type::ItemContentTypeChangedInvalidSometimes:
            {
                const CDictItem* initial_dict_item = assert_cast<const CDictItem*>(difference.initial_dict_element);
                const CDictItem* final_dict_item = assert_cast<const CDictItem*>(difference.final_dict_element);

                if( difference.type == DictionaryDifference::Type::ItemContentTypeChangedValidConversionNeeded )
                {
                    // number to string is not destructive
                    ASSERT(IsNumeric(*initial_dict_item) && IsString(*final_dict_item));
                }

                else // if( difference.type == DictionaryDifference::Type::ItemContentTypeChangedInvalidSometimes )
                {
                    // string to number is destructive
                    if( IsString(*initial_dict_item) && IsNumeric(*final_dict_item) )
                    {
                        include_difference = true;
                    }

                    // binary to binary conversions are not destructive
                    else
                    {
                        ASSERT(IsBinary(*initial_dict_item) && IsBinary(*final_dict_item));
                    }
                }

                break;
            }
        }

        if( include_difference )
            differences.emplace_back(difference);
    }

    return differences;
}
