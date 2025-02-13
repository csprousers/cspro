#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyPrinter.h>

struct FrequencyRowStatistics;
struct FrequencyTable;
class JsonWriter;


class ZFREQO_API JsonFrequencyPrinter : public FrequencyPrinter
{
protected:
    JsonFrequencyPrinter();
    
public:
    JsonFrequencyPrinter(JsonWriter& json_writer);

    void StartFrequencyGroup() override { }

    void Print(const FrequencyTable& frequency_table) override;

private:
    void PrintRowsAndTotal(const FrequencyTable& frequency_table);

    void PrintCountAndPercents(double count, const FrequencyRowStatistics& frequency_row_statistics);

    void PrintStatistics(const FrequencyTable& frequency_table);

    template<typename CF>
    void PrintStatisticsCategories(size_t number_defined_categories, CF callback_function);

protected:
    JsonWriter* m_jsonWriter;
};
