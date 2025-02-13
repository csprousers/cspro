#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyEntry.h>
#include <zFreqO/FrequencyPrinterOptions.h>

class Serializer;


class ZFREQO_API Frequency
{
public:
    std::optional<int> GetNamedFrequencySymbolIndex() const { return m_namedFrequencySymbolIndex; }
    void SetNamedFrequencySymbolIndex(int symbol_index)     { m_namedFrequencySymbolIndex = symbol_index; }

    const std::vector<FrequencyEntry>& GetFrequencyEntries() const { return m_frequencyEntries; }
    void AddFrequencyEntry(const FrequencyEntry& frequency_entry);

    FrequencyPrinterOptions& GetFrequencyPrinterOptions()             { return m_frequencyPrinterOptions; }
    const FrequencyPrinterOptions& GetFrequencyPrinterOptions() const { return m_frequencyPrinterOptions; }
    void SetFrequencyPrinterOptions(FrequencyPrinterOptions options)  { m_frequencyPrinterOptions = std::move(options); }

    std::optional<int> GetSymbolIndexOfSingleFrequencyVariable() const { return m_symbolIndexOfSingleFrequencyVariable; }

    void serialize(Serializer& ar);

private:
    std::optional<int> m_namedFrequencySymbolIndex;
    std::vector<FrequencyEntry> m_frequencyEntries;
    FrequencyPrinterOptions m_frequencyPrinterOptions;
    std::optional<int> m_symbolIndexOfSingleFrequencyVariable;
};
