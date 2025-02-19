#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyPrinter.h>

struct FrequencyRowStatistics;
struct FrequencyTable;


class ZFREQO_API JsonFrequencyPrinter : public FrequencyPrinter
{
public:
    JsonFrequencyPrinter(cs::non_null_shared_or_raw_ptr<JsonWriter> json_writer);

    void StartFrequencyGroup() override { }

    void Print(const FrequencyTable& frequency_table) override;

private:
    void PrintRowsAndTotal(const FrequencyTable& frequency_table);

    void PrintCountAndPercents(double count, const FrequencyRowStatistics& frequency_row_statistics);

    void PrintStatistics(const FrequencyTable& frequency_table);

    template<typename CF>
    void PrintStatisticsCategories(size_t number_defined_categories, const CF& callback_function);

protected:
    cs::non_null_shared_or_raw_ptr<JsonWriter> m_jsonWriter;
};
