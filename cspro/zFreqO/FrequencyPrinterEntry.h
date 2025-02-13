#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyCounter.h>

class CDictItem;
class DictValueSet;
class FrequencyPrinter;
class FrequencyPrinterOptions;
struct FrequencyTable;


template<typename ValueType, typename CountType>
class ZFREQO_API FrequencyPrinterEntry
{
public:
    FrequencyPrinterEntry(std::shared_ptr<const FrequencyCounter<ValueType, CountType>> frequency_counter,
                          std::wstring symbol_name);

    FrequencyPrinterEntry(std::shared_ptr<const FrequencyCounter<ValueType, CountType>> frequency_counter,
                          const CDictItem& dict_item, const DictValueSet* current_dict_value_set, std::optional<size_t> record_occurrence,
                          std::optional<size_t> item_subitem_occurrence);

    bool Compare(const FrequencyPrinterEntry& rhs) const;

    void Print(FrequencyPrinter& frequency_printer, std::wstring frequency_name, const FrequencyPrinterOptions& frequency_printer_options);

    std::unique_ptr<FrequencyTable> CreateFrequencyTable(std::wstring frequency_name, const FrequencyPrinterOptions& frequency_printer_options,
                                                         const DictValueSet* dict_value_set, bool distinct);

private:
    void SortFrequencyRows(FrequencyTable& frequency_table);

    void AddTitle(FrequencyTable& frequency_table);

    void AddAdditionalDisplayInformation(FrequencyTable& frequency_table);

    void AddFrequencyRowStatistics(FrequencyTable& frequency_table);

private:
    std::shared_ptr<const FrequencyCounter<ValueType, CountType>> m_frequencyCounter;
    std::wstring m_symbolName;
    const CDictItem* m_dictItem;
    const DictValueSet* m_currentDictValueSet;
    std::optional<size_t> m_recordOccurrence;
    std::optional<size_t> m_itemSubitemOccurrence;
};
