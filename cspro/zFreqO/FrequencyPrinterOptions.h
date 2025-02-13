#pragma once

#include <zFreqO/zFreqO.h>
#include <zJson/JsonSerializer.h>

namespace Nodes { struct FrequencyParameters; }
class Serializer;


class ZFREQO_API FrequencyPrinterOptions
{
public:
    enum class SortType : int { ByValueSetOrder, ByCode, ByLabel, ByCount };

    FrequencyPrinterOptions();

    const std::vector<std::wstring>& GetHeadings() const { return m_headings; }
    void SetHeadings(std::vector<std::wstring> headings);

    bool GetDistinct() const { return m_distinct; }
    void SetDistinct()       { m_distinct = true; }

    const std::vector<int>& GetValueSetSymbolIndices() const       { return m_valueSetSymbolIndices; }
    void SetValueSetSymbolIndices(std::vector<int> symbol_indices) { m_valueSetSymbolIndices = std::move(symbol_indices); }

    bool GetPrioritizeCurrentValueSet() const { return m_prioritizeCurrentValueSet; }
    void SetPrioritizeCurrentValueSet()       { m_prioritizeCurrentValueSet = true; }

    bool GetUseAllValueSets() const { return m_useAllValueSets; }
    void SetUseAllValueSets()       { m_useAllValueSets = true; }

    bool GetShowStatistics() const { return m_showStatistics; }
    void SetShowStatistics()       { m_showStatistics = true; }

    bool GetShowNoFrequencies() const { return m_showNoFrequencies; }
    void SetShowNoFrequencies()       { m_showNoFrequencies = true; }

    bool GetUsingPercentiles() const     { return ( m_percentiles != -1 ); }
    int GetPercentiles() const           { return m_percentiles; }
    void SetPercentiles(int percentiles) { m_percentiles = percentiles; }

    bool GetShowNetPercents() const    { return m_showNetPercents; }
    void SetShowNetPercents(bool show) { m_showNetPercents = show; }

    bool GetUsingDecimals() const  { return ( m_decimals != -1 ); }
    int GetDecimals() const        { return m_decimals; }
    void SetDecimals(int decimals) { m_decimals = decimals; }

    bool GetUsingPageLength() const     { return ( m_pageLength >= 1 ); }
    int GetPageLength() const           { return m_pageLength; }
    void SetPageLength(int page_length) { m_pageLength = page_length; }

    bool IsAscendingSort() const      { return m_sortAscending; }
    void SetSortOrder(bool ascending) { m_sortAscending = ascending; }

    SortType GetSortType() const         { return m_sortType; }
    void SetSortType(SortType sort_type) { m_sortType = sort_type; }

    static int GetSortOrderAndTypeAsInt(bool ascending, SortType sort_type);

    void ApplyFrequencyParametersNode(const Nodes::FrequencyParameters& frequency_parameters_node);

    void serialize(Serializer& ar);

private:
    std::vector<std::wstring> m_headings;
    bool m_distinct;
    std::vector<int> m_valueSetSymbolIndices;
    bool m_prioritizeCurrentValueSet; // only set at runtime
    bool m_useAllValueSets;
    bool m_showStatistics;
    bool m_showNoFrequencies;
    int m_percentiles;
    bool m_showNetPercents;
    int m_decimals;
    int m_pageLength;
    bool m_sortAscending;
    SortType m_sortType;
};


DECLARE_ENUM_JSON_SERIALIZER_CLASS(FrequencyPrinterOptions::SortType, ZFREQO_API)
