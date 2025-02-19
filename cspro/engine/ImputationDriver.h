#pragma once

#include <zFreqO/FrequencyCounter.h>

class CaseItem;
class CaseRecord;
class CEngineDriver;
class CIntDriver;
struct EngineData;


template<typename T>
struct ImputationFrequency
{
    using FrequencyCounterT = typename std::conditional<std::is_same_v<T, std::wstring>, std::string, T>::type; // UTF8_TODO remove

    std::shared_ptr<Imputation> imputation;
    std::shared_ptr<FrequencyCounter<FrequencyCounterT, size_t>> frequency_counter;
};


struct StatRecord
{
    CaseRecord* case_record;
    const CaseItem* initial_case_item;
    const CaseItem* imputed_case_item;
    std::vector<const CaseItem*> key_case_items;
    std::vector<const CaseItem*> stat_case_items;
    const CaseItem* line_number_case_item;
    const CaseItem* compilation_unit_case_item;
};


class ImputationDriver
{
public:
    ImputationDriver(CIntDriver& interpreter);
    virtual ~ImputationDriver();

    template<typename T>
    auto& GetImputationFrequency(size_t index)
    {
        if constexpr(std::is_same_v<T, double>)
        {
            return m_numericImputationFrequencies[index];
        }

        else
        {
            return m_stringImputationFrequencies[index];
        }
    }

    template<typename T>
    void ProcessStat(const Imputation& imputation, const T& initial_value, const T& imputed_value, const Nodes::List& stat_variable_compilations);

private:
    template<typename T>
    void SetUpImputationFrequency(std::vector<ImputationFrequency<T>>& imputation_frequencies, std::shared_ptr<Imputation> imputation);

    void WriteFrequencies();

    void SetUpStatDataFile();

    void WriteStatCase();

private:
    CIntDriver* m_interpreter;
    EngineData* m_engineData;
    CEngineDriver* m_pEngineDriver;

    // frequencies
    std::vector<ImputationFrequency<double>> m_numericImputationFrequencies;
    std::vector<ImputationFrequency<std::string>> m_stringImputationFrequencies;
    std::vector<bool> m_imputationFrequenciesPrintingOrder; // true = numeric

    // stat data
    std::shared_ptr<const CDataDict> m_statDictionary;
    std::shared_ptr<const CaseAccess> m_statCaseAccess;
    std::unique_ptr<DataRepository> m_statDataRepository;

    std::unique_ptr<Case> m_statCase;
    CaseRecord* m_statCaseIdRecord;
    std::vector<StatRecord> m_statRecords;
    std::string m_statCaseUuid;
    double m_statCaseKeyIncrementer;
};
