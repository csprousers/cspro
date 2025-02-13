#include "stdafx.h"
#include "FrequencyPrinterOptions.h"
#include <zToolsO/Serializer.h>
#include <zEngineO/Nodes/Frequency.h>


namespace
{
    constexpr int SortAscendingFlag = 0x80000000;
    constexpr int SortTypeMask      = 0x000000FF;
}


FrequencyPrinterOptions::FrequencyPrinterOptions()
    :   m_distinct(false),
        m_prioritizeCurrentValueSet(false),
        m_useAllValueSets(false),
        m_showStatistics(false),
        m_showNoFrequencies(false),
        m_percentiles(-1),
        m_showNetPercents(true),
        m_decimals(-1),
        m_pageLength(-1),
        m_sortAscending(true),
        m_sortType(SortType::ByCode)
{
}


void FrequencyPrinterOptions::SetHeadings(std::vector<std::wstring> headings)
{
    m_headings = std::move(headings);
    SO::MakeSplitVectorStringsOnNewlines(m_headings);
}


int FrequencyPrinterOptions::GetSortOrderAndTypeAsInt(bool ascending, SortType sort_type)
{
    return ( ascending ? SortAscendingFlag : 0 ) | ( static_cast<int>(sort_type) & SortTypeMask );
}


void FrequencyPrinterOptions::ApplyFrequencyParametersNode(const Nodes::FrequencyParameters& frequency_parameters_node)
{
    if( frequency_parameters_node.distinct != -1 )
        SetDistinct();

    if( frequency_parameters_node.use_all_value_sets != -1 )
        SetUseAllValueSets();

    if( frequency_parameters_node.show_statistics != -1 )
        SetShowStatistics();

    if( frequency_parameters_node.show_no_frequencies != -1 )
        SetShowNoFrequencies();

    if( frequency_parameters_node.percentiles != -1 )
        SetPercentiles(frequency_parameters_node.percentiles);

    if( frequency_parameters_node.show_no_net_percents != -1 )
        SetShowNetPercents(false);

    if( frequency_parameters_node.decimals != -1 )
        SetDecimals(frequency_parameters_node.decimals);

    if( frequency_parameters_node.page_length != -1 )
        SetPageLength(frequency_parameters_node.page_length);

    if( frequency_parameters_node.sort_order_and_type != -1 )
    {
        SetSortOrder(( frequency_parameters_node.sort_order_and_type & SortAscendingFlag ) != 0);
        SetSortType(static_cast<SortType>(frequency_parameters_node.sort_order_and_type & SortTypeMask));
    }
}


void FrequencyPrinterOptions::serialize(Serializer& ar)
{
    ar & m_headings
       & m_distinct
       & m_valueSetSymbolIndices
       & m_useAllValueSets
       & m_showStatistics
       & m_showNoFrequencies
       & m_percentiles
       & m_showNetPercents
       & m_decimals
       & m_pageLength
       & m_sortAscending;
    ar.SerializeEnum(m_sortType);
}


DEFINE_ENUM_JSON_SERIALIZER_CLASS(FrequencyPrinterOptions::SortType,
    { FrequencyPrinterOptions::SortType::ByValueSetOrder, _T("valueSet") },
    { FrequencyPrinterOptions::SortType::ByCode,          _T("code") },
    { FrequencyPrinterOptions::SortType::ByLabel,         _T("label") },
    { FrequencyPrinterOptions::SortType::ByCount,         _T("freq") })
