#pragma once

#include <zDictO/DDClass.h>


namespace DictionaryIterator
{
    // --------------------------------------------------------------------------
    // a way to iterate over the elements of a dictionary using a subclass
    // --------------------------------------------------------------------------
    class Iterator
    {
    public:
        Iterator()
            :   m_dictionary(nullptr),
                m_currentLevel(nullptr),
                m_currentRecord(nullptr),
                m_currentItem(nullptr),
                m_currentValueSet(nullptr)    
        {
        }

        virtual ~Iterator() { }

        void Iterate(CDataDict& dictionary)
        {
            m_dictionary = &dictionary;

            ProcessDictionary(dictionary);

            for( DictLevel& dict_level : dictionary.GetLevels() )
            {
                m_currentLevel = &dict_level;
                ProcessLevel(dict_level);

                for( int r = -1; r < m_currentLevel->GetNumRecords(); ++r )
                {
                    m_currentRecord = ( r == -1 ) ? m_currentLevel->GetIdItemsRec() : m_currentLevel->GetRecord(r);
                    ProcessRecord(*m_currentRecord);

                    for( int i = 0; i < m_currentRecord->GetNumItems(); ++i )
                    {
                        m_currentItem = m_currentRecord->GetItem(i);
                        ProcessItem(*m_currentItem);

                        for( auto& dict_value_set : m_currentItem->GetValueSets() )
                        {
                            m_currentValueSet = &dict_value_set;
                            ProcessValueSet(dict_value_set);

                            for( auto& dict_value : dict_value_set.GetValues() )
                                ProcessValue(dict_value);
                        }

                        m_currentValueSet = nullptr;
                    }

                    m_currentItem = nullptr;
                }

                m_currentRecord = nullptr;
            }
        }

    protected:
        virtual void ProcessDictionary(CDataDict& /*dictionary*/) { }
        virtual void ProcessLevel(DictLevel& /*dict_level*/) { }
        virtual void ProcessRecord(CDictRecord& /*dict_record*/) { }
        virtual void ProcessItem(CDictItem& /*dict_item*/) { }
        virtual void ProcessValueSet(DictValueSet& /*dict_value_set*/) { }
        virtual void ProcessValue(DictValue& /*dict_value*/) { }

    protected:
        CDataDict* m_dictionary;
        DictLevel* m_currentLevel;
        CDictRecord* m_currentRecord;
        CDictItem* m_currentItem;
        DictValueSet* m_currentValueSet;
    };



    // --------------------------------------------------------------------------
    // a way to iterate over the named elements of a dictionary using a subclass
    // --------------------------------------------------------------------------
    class NamedElementIterator : public Iterator
    {
    protected:
        virtual void ProcessNamedElement(DictNamedBase& dict_element) = 0;

    private:
        void ProcessDictionary(CDataDict& dictionary) override      { ProcessNamedElement(dictionary); }
        void ProcessLevel(DictLevel& dict_level) override           { ProcessNamedElement(dict_level); }
        void ProcessRecord(CDictRecord& dict_record) override       { ProcessNamedElement(dict_record); }
        void ProcessItem(CDictItem& dict_item) override             { ProcessNamedElement(dict_item); }
        void ProcessValueSet(DictValueSet& dict_value_set) override { ProcessNamedElement(dict_value_set); }
    };



    // --------------------------------------------------------------------------
    // a way to iterate over a value set and its values using a subclass
    // --------------------------------------------------------------------------
    class ValueSetIterator
    {
    public:
        ValueSetIterator(const DictValueSet& dict_value_set)
            :   m_dictValueSet(dict_value_set),
                m_dictValue(nullptr)
        {
        }

        virtual ~ValueSetIterator() { }

        void Iterate()
        {
            ProcessValueSet(m_dictValueSet);

            for( const DictValue& dict_value : m_dictValueSet.GetValues() )
            {
                m_dictValue = &dict_value;
                ProcessValueSetValue(dict_value);

                for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
                    ProcessValueSetValuePair(dict_value_pair);
            }
        }

        template<typename CF>
        static void IterateValueSetValuePairs(const DictValueSet& dict_value_set, CF callback_function)
        {
            for( const DictValue& dict_value : dict_value_set.GetValues() )
            {
                for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
                    callback_function(dict_value, dict_value_pair);
            }
        }

    protected:
        virtual void ProcessValueSet(const DictValueSet& /*dict_value_set*/) { }
        virtual void ProcessValueSetValue(const DictValue& /*dict_value*/) { }
        virtual void ProcessValueSetValuePair(const DictValuePair& /*dict_value_pair*/) { }

    protected:
        const DictValueSet& m_dictValueSet;
        const DictValue* m_dictValue;
    };



    // --------------------------------------------------------------------------
    // a way to iterate over the elements of a dictionary using a callback
    // function
    // --------------------------------------------------------------------------
    template<typename UpToClass, typename DT, typename CF>
    void ForeachDictBaseUpTo(DT& dictionary, CF callback_function)
    {
        static_assert(std::is_same_v<UpToClass, DictLevel> ||
                      std::is_same_v<UpToClass, CDictRecord> || 
                      std::is_same_v<UpToClass, CDictItem> || 
                      std::is_same_v<UpToClass, DictValueSet> || 
                      std::is_same_v<UpToClass, DictValue>);

        callback_function(dictionary);

        for( auto& dict_level : dictionary.GetLevels() )
        {
            callback_function(dict_level);

            if constexpr(!std::is_same_v<UpToClass, DictLevel>)
            {
                for( int r = -1; r < dict_level.GetNumRecords(); ++r )
                {
                    auto* dict_record = dict_level.GetRecord(( r == -1 ) ? COMMON : r);
                    callback_function(*dict_record);

                    if constexpr(!std::is_same_v<UpToClass, CDictRecord>)
                    {
                        for( int i = 0; i < dict_record->GetNumItems(); ++i )
                        {
                            auto* dict_item = dict_record->GetItem(i);
                            callback_function(*dict_item);

                            if constexpr(!std::is_same_v<UpToClass, CDictItem>)
                            {
                                for( auto& dict_value_set : dict_item->GetValueSets() )
                                {
                                    callback_function(dict_value_set);

                                    if constexpr(!std::is_same_v<UpToClass, DictValueSet>)
                                    {
                                        for( auto& dict_value : dict_value_set.GetValues() )
                                            callback_function(dict_value);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    template<typename DT, typename CF>
    void ForeachDictBase(DT& dictionary, CF callback_function)
    {
        ForeachDictBaseUpTo<DictValue>(dictionary, callback_function);
    }

    template<typename DT, typename CF>
    void ForeachDictNamedBase(DT& dictionary, CF callback_function)
    {
        ForeachDictBaseUpTo<DictValueSet>(dictionary, callback_function);
    }



    // --------------------------------------------------------------------------
    // a way to iterate over only certain elements of a dictionary using a
    // callback function
    // --------------------------------------------------------------------------
    template<typename IterateClass, typename DT, typename CF>
    void Foreach(DT& dictionary, CF callback_function)
    {
        static_assert(std::is_same_v<IterateClass, DictLevel> ||
                      std::is_same_v<IterateClass, CDictRecord> || 
                      std::is_same_v<IterateClass, CDictItem> || 
                      std::is_same_v<IterateClass, DictValueSet> || 
                      std::is_same_v<IterateClass, DictValue>);

        for( auto& dict_level : dictionary.GetLevels() )
        {
            if constexpr(std::is_same_v<IterateClass, DictLevel>)
            {
                callback_function(dict_level);
            }

            else
            {
                for( int r = -1; r < dict_level.GetNumRecords(); ++r )
                {
                    auto* dict_record = dict_level.GetRecord(( r == -1 ) ? COMMON : r);

                    if constexpr(std::is_same_v<IterateClass, CDictRecord>)
                    {
                        callback_function(*dict_record);
                    }

                    else                    
                    {
                        for( int i = 0; i < dict_record->GetNumItems(); ++i )
                        {
                            auto* dict_item = dict_record->GetItem(i);

                            if constexpr(std::is_same_v<IterateClass, CDictItem>)
                            {
                                callback_function(*dict_item);
                            }

                            else                            
                            {
                                for( auto& dict_value_set : dict_item->GetValueSets() )
                                {
                                    if constexpr(std::is_same_v<IterateClass, DictValueSet>)
                                    {
                                        callback_function(dict_value_set);
                                    }

                                    else
                                    {
                                        for( auto& dict_value : dict_value_set.GetValues() )
                                            callback_function(dict_value);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }



    // --------------------------------------------------------------------------
    // a way to iterate over only certain elements of a dictionary using a
    // callback function; the callback should return true to keep processing
    // --------------------------------------------------------------------------
    template<typename IterateClass, typename DT, typename CF>
    void ForeachWhile(DT& dictionary, CF callback_function)
    {
        static_assert(std::is_same_v<IterateClass, DictLevel> ||
                      std::is_same_v<IterateClass, CDictRecord> || 
                      std::is_same_v<IterateClass, CDictItem> || 
                      std::is_same_v<IterateClass, DictValueSet> || 
                      std::is_same_v<IterateClass, DictValue>);

        for( auto& dict_level : dictionary.GetLevels() )
        {
            if constexpr(std::is_same_v<IterateClass, DictLevel>)
            {
                if( !callback_function(dict_level) )
                    return;
            }

            else
            {
                for( int r = -1; r < dict_level.GetNumRecords(); ++r )
                {
                    auto* dict_record = dict_level.GetRecord(( r == -1 ) ? COMMON : r);

                    if constexpr(std::is_same_v<IterateClass, CDictRecord>)
                    {
                        if( !callback_function(*dict_record) )
                            return;
                    }

                    else                    
                    {
                        for( int i = 0; i < dict_record->GetNumItems(); ++i )
                        {
                            auto* dict_item = dict_record->GetItem(i);

                            if constexpr(std::is_same_v<IterateClass, CDictItem>)
                            {
                                if( !callback_function(*dict_item) )
                                    return;
                            }

                            else                            
                            {
                                for( auto& dict_value_set : dict_item->GetValueSets() )
                                {
                                    if constexpr(std::is_same_v<IterateClass, DictValueSet>)
                                    {
                                        if( !callback_function(dict_value_set) )
                                            return;
                                    }

                                    else
                                    {
                                        for( auto& dict_value : dict_value_set.GetValues() )
                                        {
                                            if( !callback_function(dict_value) )
                                                return;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }



    // --------------------------------------------------------------------------
    // a way to iterate over all of the LabelSet objects in a dictionary using a
    // callback function
    // --------------------------------------------------------------------------
    template<typename DT, typename CF>
    void ForeachLabelSet(DT& dictionary, CF callback_function)
    {
        callback_function(dictionary.GetLabelSet());

        for( auto& dict_level : dictionary.GetLevels() )
        {
            callback_function(dict_level.GetLabelSet());

            for( int r = -1; r < dict_level.GetNumRecords(); ++r )
            {
                auto* dict_record = dict_level.GetRecord(( r == -1 ) ? COMMON : r);
                callback_function(dict_record->GetLabelSet());

                for( auto& occurrence_label_set : dict_record->GetOccurrenceLabels().GetLabels() )
                    callback_function(occurrence_label_set);

                for( int i = 0; i < dict_record->GetNumItems(); ++i )
                {
                    auto* dict_item = dict_record->GetItem(i);
                    callback_function(dict_item->GetLabelSet());

                    for( auto& occurrence_label_set : dict_item->GetOccurrenceLabels().GetLabels() )
                        callback_function(occurrence_label_set);

                    for( auto& dict_value_set : dict_item->GetValueSets() )
                    {
                        callback_function(dict_value_set.GetLabelSet());

                        for( auto& dict_value : dict_value_set.GetValues() )
                            callback_function(dict_value.GetLabelSet());
                    }
                }
            }
        }
    }
}
