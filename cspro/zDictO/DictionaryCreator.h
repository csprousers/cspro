#pragma once

#include <zDictO/DDClass.h>


// this simple class creates a single-record dictionary based on an existing dictionary

class DictionaryCreator
{
public:
    DictionaryCreator(const CDataDict& source_dictionary, const std::string& name_prefix, const std::string& label, size_t maximum_record_occurrences);

    DictionaryCreator& AddItem(std::string name, std::string label, ContentType content_type, int item_length);

    template<typename T>
    DictionaryCreator& AddValueSet(const std::string& item_name, const T& values);

    std::unique_ptr<CDataDict> ReleaseDictionary();

private:
    std::unique_ptr<CDataDict> m_dictionary;
    DictLevel m_dictLevel;
    CDictRecord m_dictRecord;
    int m_itemPosition;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline DictionaryCreator::DictionaryCreator(const CDataDict& source_dictionary, const std::string& name_prefix, const std::string& label,
                                            const size_t maximum_record_occurrences)
    :   m_dictionary(std::make_unique<CDataDict>()),
        m_itemPosition(1)
{
    m_dictionary->SetName(name_prefix + source_dictionary.GetName());
    m_dictionary->SetLabel(UTF8_TODO::GetCString(FormatText("%s (%s Dictionary)", UTF8_TODO::GetUtf8(source_dictionary.GetLabel()).c_str(), label.c_str())));
    m_dictionary->SetPosRelative(true);
    m_dictionary->SetRecTypeLen(0);
    m_dictionary->SetRecTypeStart(0);
    m_dictionary->CopyDictionarySettings(source_dictionary);

    const DictLevel& source_dict_level = source_dictionary.GetLevel(0);

    m_dictLevel.SetName(name_prefix + source_dict_level.GetName());
    m_dictLevel.SetLabel(UTF8_TODO::GetCString(FormatText("%s (%s Level)", UTF8_TODO::GetUtf8(source_dict_level.GetLabel()).c_str(), label.c_str())));

    m_dictRecord.SetName(name_prefix + "REC");
    m_dictRecord.SetLabel(UTF8_TODO::GetCString(FormatText("%s (%s Record)", UTF8_TODO::GetUtf8(source_dictionary.GetLabel()).c_str(), label.c_str())));
    m_dictRecord.SetMaxRecs(maximum_record_occurrences);

    // add the ID items
    auto add_id_item = [&](CDictRecord& record_for_id_item, const CDictItem& id_item)
    {
        CDictItem dict_item(id_item);

        dict_item.SetName(name_prefix + dict_item.GetName());
        dict_item.SetStart(m_itemPosition);

        record_for_id_item.AddItem(&dict_item);

        m_itemPosition += dict_item.GetLen();
    };

    for( const CDictItem* const id_item : source_dictionary.GetIdItems() )
    {
        add_id_item(( id_item->GetLevel()->GetLevelNumber() == 0 ) ? *m_dictLevel.GetIdItemsRec() : m_dictRecord, *id_item);
    }
}


inline DictionaryCreator& DictionaryCreator::AddItem(std::string name, std::string label, const ContentType content_type, const int item_length)
{
    CDictItem dict_item;

    dict_item.SetName(std::move(name));
    dict_item.SetLabel(UTF8_TODO::GetCString(std::move(label)));
    dict_item.SetContentType(content_type);
    dict_item.SetStart(m_itemPosition);
    dict_item.SetLen(item_length);
    dict_item.SetZeroFill(m_dictionary->IsZeroFill());

    m_dictRecord.AddItem(&dict_item);

    m_itemPosition += item_length;

    return *this;
}


template<typename T>
DictionaryCreator& DictionaryCreator::AddValueSet(const std::string& item_name, const T& values)
{
    CDictItem* const dict_item = m_dictRecord.FindItem(item_name);
    ASSERT(dict_item != nullptr && dict_item->GetContentType() == ContentType::Numeric);

    DictValueSet dict_value_set;
    dict_value_set.SetName(dict_item->GetName() + "_VS1");
    dict_value_set.SetLabel(dict_item->GetLabel());

    for( const auto& [label, value] : values )
    {
        DictValue dict_value;
        dict_value.SetLabel(label);

        dict_value.AddValuePair(DictValuePair(FormatText("%*.0f", dict_item->GetLen(), value)));

        dict_value_set.AddValue(std::move(dict_value));
    }

    dict_item->AddValueSet(std::move(dict_value_set));

    return *this;
}


inline std::unique_ptr<CDataDict> DictionaryCreator::ReleaseDictionary()
{
    // finalize the dictionary
    m_dictRecord.SetRecLen(m_itemPosition - 1);

    m_dictLevel.AddRecord(&m_dictRecord);
    m_dictionary->AddLevel(std::move(m_dictLevel));

    m_dictionary->UpdatePointers();

    return std::move(m_dictionary);
}
