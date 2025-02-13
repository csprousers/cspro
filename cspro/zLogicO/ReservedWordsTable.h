#pragma once

#include <zToolsO/StringNoCase.h>
#include <zToolsO/span.h>


namespace Logic
{
    // --------------------------------------------------------------------------
    // ReservedWordsTable
    // --------------------------------------------------------------------------

    template<typename T>
    class ReservedWordsTable
    {
    public:
        ReservedWordsTable(cs::span<const T> entries);

        bool IsEntry(wstring_view text, const T** entry_details) const;

        template<typename P>
        const TCHAR* GetName(const P& func) const;

        const std::map<StringNoCase, const T*>& GetTable() const { return m_table; }

    private:
        std::map<StringNoCase, const T*> m_table;
    };


    // --------------------------------------------------------------------------
    // MultipleReservedWordsTable
    // --------------------------------------------------------------------------

    template<typename T>
    class MultipleReservedWordsTable
    {
    public:
        template<typename VT>
        MultipleReservedWordsTable(cs::span<const VT> entries);

        template<typename Filter1, typename Filter2>
        bool IsEntry(wstring_view text, const Filter1 T::*table_filter_value, const Filter2& filter, const T** entry_details) const;

        const std::map<StringNoCase, std::vector<const T*>>& GetTable() const { return m_table; }

    private:
        std::map<StringNoCase, std::vector<const T*>> m_table;
    };
}



// --------------------------------------------------------------------------
// ReservedWordsTable: inline implementations
// --------------------------------------------------------------------------

template<typename T>
Logic::ReservedWordsTable<T>::ReservedWordsTable(cs::span<const T> entries)
{
    for( const T& entry : entries )
        m_table.try_emplace(entry.name, &entry);
}


template<typename T>
bool Logic::ReservedWordsTable<T>::IsEntry(wstring_view text, const T** entry_details) const
{
    const auto& table_search = m_table.find(text);

    if( table_search == m_table.end() )
        return false;

    if( entry_details != nullptr )
        *entry_details = table_search->second;

    return true;
}


template<typename T>
template<typename P>
const TCHAR* Logic::ReservedWordsTable<T>::GetName(const P& func) const
{
    for( const auto& [name, entry] : m_table )
    {
        if( func(*entry) )
            return name.c_str();
    }

    return ReturnProgrammingError(_T(""));
}



// --------------------------------------------------------------------------
// MultipleReservedWordsTable: inline implementations
// --------------------------------------------------------------------------

template<typename T>
template<typename VT>
Logic::MultipleReservedWordsTable<T>::MultipleReservedWordsTable(cs::span<const VT> entries)
{
    for( const VT& entry_value_or_pointer : entries )
    {
        const T* entry;

        if constexpr(IsPointer<VT>())
        {
            entry = entry_value_or_pointer;
        }

        else
        {
            entry = &entry_value_or_pointer;
        }

        auto name_lookup = m_table.find(entry->name);

        std::vector<const T*>& entries_for_name = ( name_lookup != m_table.end() ) ? name_lookup->second :
                                                                                     m_table.try_emplace(entry->name, std::vector<const T*>()).first->second;

        entries_for_name.emplace_back(entry);
    }
}


template<typename T>
template<typename Filter1, typename Filter2>
bool Logic::MultipleReservedWordsTable<T>::IsEntry(wstring_view text, const Filter1 T::*table_filter_value, const Filter2& filter, const T** entry_details) const
{
    const auto& name_lookup = m_table.find(text);

    if( name_lookup != m_table.end() )
    {
        for( const auto& details_itr : name_lookup->second )
        {
            if( details_itr->*table_filter_value == filter )
            {
                if( entry_details != nullptr )
                    *entry_details = details_itr;

                return true;
            }
        }
    }

    return false;
}
