#pragma once

#include <zAppO/LanguageSerializerHelper.h>


class DictionarySerializerHelper : public LanguageSerializerHelper
{
public:
    DictionarySerializerHelper(const CDataDict& dictionary)
        :   LanguageSerializerHelper(dictionary.GetLanguages()),
            m_dictionary(dictionary),
            m_currentLevel(nullptr),
            m_currentRecord(nullptr),
            m_currentItem(nullptr),
            m_currentValueSet(nullptr),
            m_itemStart(0),
            m_lastParentItemStart(0),
            m_usesBinaryItems(false)
    {
    }

    const CDataDict& GetDictionary() const                      { return m_dictionary; }
                                                                
    const DictLevel* GetCurrentLevel() const                    { return m_currentLevel; }
    void SetCurrentLevel(const DictLevel& dict_level)           { m_currentLevel = &dict_level; }
                                                                
    const CDictRecord* GetCurrentRecord() const                 { return m_currentRecord; }
    void SetCurrentRecord(const CDictRecord& dict_record)       { m_currentRecord = &dict_record; }
                                                                
    const CDictItem* GetCurrentItem() const                     { return m_currentItem; }
    void SetCurrentItem(const CDictItem& dict_item)             { m_currentItem = &dict_item; }

    const DictValueSet* GetCurrentValueSet() const              { return m_currentValueSet; }
    void SetCurrentValueSet(const DictValueSet& dict_value_set) { m_currentValueSet = &dict_value_set; }

    bool GetUsesBinaryItems() const { return m_usesBinaryItems; } // BINARY_TYPES_TO_ENGINE_TODO
    void SetUsesBinaryItems()       { m_usesBinaryItems = true; }


    // item start position helpers
    // --------------------------------------------------------------------------
    unsigned GetItemStart() const           { return m_itemStart; }
    void SetItemStart(unsigned start)       { m_itemStart = start; }

    unsigned GetLastParentItemStart() const { return m_lastParentItemStart; }

    void IncrementItemStart(const CDictItem& dict_item)
    {
        ASSERT(!dict_item.IsSubitem());
        m_lastParentItemStart = dict_item.GetStart();
        m_itemStart += dict_item.GetLen() * dict_item.GetOccurs();
    }


    // linked value set helpers
    // --------------------------------------------------------------------------
    bool HasValueSetBeenSerialized(const std::wstring& serialized_link) const
    {
        return ( m_alreadySerializedValueSetLinks.find(serialized_link) != m_alreadySerializedValueSetLinks.cend() );
    }

    void MarkValueSetAsSerialized(std::wstring serialized_link)
    {
        ASSERT(!HasValueSetBeenSerialized(serialized_link));
        m_alreadySerializedValueSetLinks.insert(std::move(serialized_link));
    }


private:
    const CDataDict& m_dictionary;
    const DictLevel* m_currentLevel;
    const CDictRecord* m_currentRecord;
    const CDictItem* m_currentItem;
    const DictValueSet* m_currentValueSet;

    unsigned m_itemStart;
    unsigned m_lastParentItemStart;

    std::set<std::wstring> m_alreadySerializedValueSetLinks;

    bool m_usesBinaryItems;
};
