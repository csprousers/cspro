#pragma once

#include <zDictO/DDClass.h>


// this simple class creates a single-record dictionary based on an existing dictionary

class DictionaryCreator
{
public:
    DictionaryCreator(const CDataDict& source_dictionary, const CString& name_prefix,
                      const CString& label, size_t maximum_record_occurrences)
        :   m_dictionary(std::make_unique<CDataDict>()),
            m_itemPosition(1)
    {
        m_dictionary->SetName(name_prefix + source_dictionary.GetName());
        m_dictionary->SetLabel(FormatText(_T("%s (%s Dictionary)"), (LPCTSTR)source_dictionary.GetLabel(), (LPCTSTR)label));
        m_dictionary->SetPosRelative(true);
        m_dictionary->SetRecTypeLen(0);
        m_dictionary->SetRecTypeStart(0);
        m_dictionary->CopyDictionarySettings(source_dictionary);

        const DictLevel& source_dict_level = source_dictionary.GetLevel(0);

        m_dictLevel.SetName(name_prefix + source_dict_level.GetName());
        m_dictLevel.SetLabel(FormatText(_T("%s (%s Level)"), (LPCTSTR)source_dict_level.GetLabel(), (LPCTSTR)label));

        m_dictRecord.SetName(name_prefix + _T("REC"));
        m_dictRecord.SetLabel(FormatText(_T("%s (%s Record)"), (LPCTSTR)source_dictionary.GetLabel(), (LPCTSTR)label));
        m_dictRecord.SetMaxRecs(maximum_record_occurrences);

        // add the ID items
        auto add_id_item = [&](CDictRecord& record_for_id_item, const CDictItem& id_item)
        {
            CDictItem item(id_item);

            item.SetName(name_prefix + item.GetName());
            item.SetStart(m_itemPosition);

            record_for_id_item.AddItem(&item);

            m_itemPosition += item.GetLen();
        };

        for( const CDictItem* id_item : source_dictionary.GetIdItems() )
        {
            add_id_item(( id_item->GetLevel()->GetLevelNumber() == 0 ) ? *m_dictLevel.GetIdItemsRec() : m_dictRecord, *id_item);
        }
    }

    DictionaryCreator& AddItem(const CString& name, const CString& label, ContentType content_type, int item_length)
    {
        CDictItem dict_item;

        dict_item.SetName(name);
        dict_item.SetLabel(label);
        dict_item.SetContentType(content_type);
        dict_item.SetStart(m_itemPosition);
        dict_item.SetLen(item_length);
        dict_item.SetZeroFill(m_dictionary->IsZeroFill());

        m_dictRecord.AddItem(&dict_item);

        m_itemPosition += item_length;

        return *this;
    };

    DictionaryCreator& AddValueSet(const CString& item_name, const std::vector<std::tuple<CString, double>>& values)
    {
        CDictItem* dict_item = m_dictRecord.FindItem(item_name);
        ASSERT(dict_item != nullptr && dict_item->GetContentType() == ContentType::Numeric);

        DictValueSet dict_value_set;
        dict_value_set.SetName(dict_item->GetName() + _T("_VS1"));
        dict_value_set.SetLabel(dict_item->GetLabel());

        for( const auto& [label, value] : values )
        {
            DictValue dict_value;
            dict_value.SetLabel(label);

            CString from_value = FormatText(_T("%*.0f"), dict_item->GetLen(), value);
            dict_value.AddValuePair(DictValuePair(from_value));

            dict_value_set.AddValue(std::move(dict_value));
        }

        dict_item->AddValueSet(std::move(dict_value_set));

        return *this;
    }

    std::unique_ptr<CDataDict> GetDictionary()
    {
        // finalize the dictionary
        m_dictRecord.SetRecLen(m_itemPosition - 1);

        m_dictLevel.AddRecord(&m_dictRecord);
        m_dictionary->AddLevel(std::move(m_dictLevel));

        m_dictionary->UpdatePointers();

        return std::move(m_dictionary);
    }

private:
    std::unique_ptr<CDataDict> m_dictionary;
    DictLevel m_dictLevel;
    CDictRecord m_dictRecord;
    int m_itemPosition;
};
