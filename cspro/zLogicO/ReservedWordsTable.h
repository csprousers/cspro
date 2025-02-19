#pragma once

#include <zToolsO/CaseInsensitiveComparer.h>
#include <zToolsO/span.h>

namespace Logic { template<typename T> class MultipleReservedWordsTable;
                  template<typename T> class ReservedWordsTable; }



// --------------------------------------------------------------------------
// ReservedWordsTable
// --------------------------------------------------------------------------

template<typename T>
class Logic::ReservedWordsTable
{
public:
    ReservedWordsTable(cs::span<const T> entries);

    bool IsEntry(std::string_view text_sv, const T** entry_details) const;

    template<typename P>
    const char* GetName(const P& func) const;

    const auto& GetTable() const { return m_table; }

private:
    std::map<std::string, const T*, cs::case_insensitive_less> m_table;
};



// --------------------------------------------------------------------------
// MultipleReservedWordsTable
// --------------------------------------------------------------------------

template<typename T>
class Logic::MultipleReservedWordsTable
{
public:
    template<typename VT>
    MultipleReservedWordsTable(cs::span<const VT> entries);

    template<typename CF>
    bool IsEntry(std::string_view text_sv, const T** entry_details, const CF& filter_callback) const;

    template<typename Filter1, typename Filter2>
    bool IsEntry(std::string_view text_sv, const Filter1 T::*table_filter_value, const Filter2& filter, const T** entry_details) const;

    const auto& GetTable() const { return m_table; }

private:
    std::map<std::string, std::vector<const T*>, cs::case_insensitive_less> m_table;
};



// --------------------------------------------------------------------------
// ReservedWordsTable: inline implementations
// --------------------------------------------------------------------------

template<typename T>
Logic::ReservedWordsTable<T>::ReservedWordsTable(const cs::span<const T> entries)
{
    for( const T& entry : entries )
        m_table.try_emplace(entry.name, &entry);
}


template<typename T>
bool Logic::ReservedWordsTable<T>::IsEntry(const std::string_view text_sv, const T** entry_details) const
{
    const auto& table_search = m_table.find(text_sv);

    if( table_search == m_table.end() )
        return false;

    if( entry_details != nullptr )
        *entry_details = table_search->second;

    return true;
}


template<typename T>
template<typename P>
const char* Logic::ReservedWordsTable<T>::GetName(const P& func) const
{
    for( const auto& [name, entry] : m_table )
    {
        if( func(*entry) )
            return name.c_str();
    }

    return ReturnProgrammingError("");
}



// --------------------------------------------------------------------------
// MultipleReservedWordsTable: inline implementations
// --------------------------------------------------------------------------

template<typename T>
template<typename VT>
Logic::MultipleReservedWordsTable<T>::MultipleReservedWordsTable(const cs::span<const VT> entries)
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

        const auto& name_lookup = m_table.find(entry->name);

        std::vector<const T*>& entries_for_name = ( name_lookup != m_table.end() ) ? name_lookup->second :
                                                                                     m_table.try_emplace(entry->name, std::vector<const T*>()).first->second;

        entries_for_name.emplace_back(entry);
    }
}


template<typename T>
template<typename CF>
bool Logic::MultipleReservedWordsTable<T>::IsEntry(const std::string_view text_sv, const T** entry_details, const CF& filter_callback) const
{
    const auto& name_lookup = m_table.find(text_sv);

    if( name_lookup != m_table.end() )
    {
        for( const auto& details_itr : name_lookup->second )
        {
            if( filter_callback(*details_itr) )
            {
                if( entry_details != nullptr )
                    *entry_details = details_itr;

                return true;
            }
        }
    }

    return false;
}


template<typename T>
template<typename Filter1, typename Filter2>
bool Logic::MultipleReservedWordsTable<T>::IsEntry(const std::string_view text_sv, const Filter1 T::*table_filter_value, const Filter2& filter, const T** entry_details) const
{
    return IsEntry(text_sv, entry_details, [&](const auto& details) { return ( details.*table_filter_value == filter ); });
}
