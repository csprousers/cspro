#pragma once

#include <zLogicO/Symbol.h>

class CEngineArea;
class CEngineDriver;
class CIntDriver;
struct EngineData;
class Frequency;
class FrequencyPrinter;
class FrequencyPrinterOptions;
class EngineFrequencyEntry;
class EngineFrequencyApplicabilityEvaluator;
class PFF;


class FrequencyDriver
{
public:
    FrequencyDriver(CIntDriver& int_driver);
    virtual ~FrequencyDriver();

    static std::unique_ptr<FrequencyPrinter> CreateFrequencyPrinter(std::wstring filename, const PFF& pff);
    std::unique_ptr<FrequencyPrinter> CreateFrequencyPrinter(std::wstring filename);

    std::shared_ptr<FrequencyPrinter> GetDefaultFrequencyPrinter();

    void ResetFrequency(size_t frequency_index, int heading_expressions_list_node_index);

    void ClearFrequencyTallies(size_t frequency_index);

    double TallyFrequency(size_t frequency_index, int weight_expression);

    void PrintFrequencies(size_t frequency_index, FrequencyPrinter& frequency_printer,
                          std::wstring frequency_name, const FrequencyPrinterOptions& frequency_printer_options);

    double GetSingleFrequencyCounterCount(int var_node_index);
    void SetSingleFrequencyCounterCount(int var_node_index, double count);
    void ModifySingleFrequencyCounterCount(int var_node_index, const std::function<void(double&)>& modify_count_function);

    // these methods are marked virtual so that they are accessible from the zEngineO project, not because they will be overriden
    virtual void CloneFrequencyInInitialState(NamedFrequency& cloned_named_frequency, size_t source_frequency_index);

    virtual void WriteJsonMetadata_subclass(const NamedFrequency& named_frequency, JsonWriter& json_writer) const;
    virtual void WriteValueToJson(const NamedFrequency& named_frequency, JsonWriter& json_writer);

private:
    const Logic::SymbolTable& GetSymbolTable() const { return m_symbolTable; }

    void CreateEngineFrequencyEntries(const Frequency& frequency);

    template<typename GetCallback, typename SetCallback>
    void UseSingleFrequencyCounterForGettingAndSetting(int var_node_index, GetCallback get_callback, SetCallback set_callback);

private:
    CEngineArea* m_pEngineArea;
    CEngineDriver* m_pEngineDriver;
    CIntDriver* m_pIntDriver;
    EngineData* m_engineData;
    const Logic::SymbolTable& m_symbolTable;

    std::shared_ptr<FrequencyPrinter> m_frequencyPrinter;

    std::vector<std::vector<std::shared_ptr<EngineFrequencyEntry>>> m_engineFrequencyEntries;
    std::vector<std::vector<std::shared_ptr<EngineFrequencyApplicabilityEvaluator>>> m_engineFrequencyApplicabilityEvaluators;
};
