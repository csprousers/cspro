#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "citer.h"
#include "FrequencyDriver.h"
#include <zEngineO/NamedFrequency.h>
#include <zEngineO/Report.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/WorkString.h>
#include <zEngineO/WorkVariable.h>
#include <zEngineO/Nodes/Frequency.h>
#include <zEngineF/EngineUI.h>
#include <zJson/Json.h>
#include <ZBRIDGEO/npff.h>
#include <zFreqO/ExcelFrequencyPrinter.h>
#include <zFreqO/Frequency.h>
#include <zFreqO/FrequencyCounter.h>
#include <zFreqO/FrequencyPrinter.h>
#include <zFreqO/FrequencyPrinterEntry.h>
#include <zFreqO/HtmlFrequencyPrinter.h>
#include <zFreqO/JsonFileFrequencyPrinter.h>
#include <zFreqO/JsonStringFrequencyPrinter.h>
#include <zFreqO/TableFrequencyPrinter.h>
#include <zFreqO/TextFileFrequencyPrinter.h>


// --------------------------------------------------------------------------
// helper functions
// --------------------------------------------------------------------------

namespace
{
    std::vector<std::wstring> EvaluateDynamicHeadings(CIntDriver* int_driver, int heading_expressions_list_node_index)
    {
        const auto& heading_expressions_list_node = int_driver->GetListNode(heading_expressions_list_node_index);
        ASSERT(heading_expressions_list_node.number_elements > 0);

        std::vector<std::wstring> headings;

        for( int i = 0; i < heading_expressions_list_node.number_elements; ++i )
            headings.emplace_back(int_driver->EvalAlphaExpr(heading_expressions_list_node.elements[i]));

        return headings;
    }

    FrequencyPrinterOptions EvaluateDynamicFrequencyPrinterOptions(CIntDriver* int_driver, const Frequency& frequency, int frequency_parameters_node_index)
    {
        FrequencyPrinterOptions frequency_printer_options = frequency.GetFrequencyPrinterOptions();
        frequency_printer_options.SetPrioritizeCurrentValueSet();

        if( frequency_parameters_node_index != -1 )
        {
            const auto& frequency_parameters_node = int_driver->GetNode<Nodes::FrequencyParameters>(frequency_parameters_node_index);

            frequency_printer_options.ApplyFrequencyParametersNode(frequency_parameters_node);

            if( frequency_parameters_node.heading_expressions_list_node != -1 )
                frequency_printer_options.SetHeadings(EvaluateDynamicHeadings(int_driver, frequency_parameters_node.heading_expressions_list_node));

            if( frequency_parameters_node.value_sets_list_node != -1 )
                frequency_printer_options.SetValueSetSymbolIndices(int_driver->GetListNodeContents(frequency_parameters_node.value_sets_list_node));
        }

        return frequency_printer_options;
    }
}



// --------------------------------------------------------------------------
// the base frequency applicability evaluator
// --------------------------------------------------------------------------

class EngineFrequencyApplicabilityEvaluator
{
public:
    virtual ~EngineFrequencyApplicabilityEvaluator() { }

    virtual bool IsApplicable() = 0;

    auto GetEngineFrequencyEntriesIterator()
    {
        return VI_V(m_engineFrequencyEntries);
    }

    void AddEngineFrequencyEntry(std::shared_ptr<EngineFrequencyEntry> engine_frequency_entry)
    {
        m_engineFrequencyEntries.emplace_back(std::move(engine_frequency_entry));
    }

private:
    std::vector<std::shared_ptr<EngineFrequencyEntry>> m_engineFrequencyEntries;
};



// --------------------------------------------------------------------------
// a frequency applicability evaluator that always returns true
// --------------------------------------------------------------------------

class AlwaysApplicableEngineFrequencyApplicabilityEvaluator : public EngineFrequencyApplicabilityEvaluator
{
public:
    bool IsApplicable() override
    {
        return true;
    }
};



// --------------------------------------------------------------------------
// a frequency applicability evaluator for singly occurring items
// --------------------------------------------------------------------------

class SinglyOccurringItemFrequencyApplicabilityEvaluator : public EngineFrequencyApplicabilityEvaluator
{
public:
    SinglyOccurringItemFrequencyApplicabilityEvaluator(const VART* pVarT)
        :   m_pGroupT(pVarT->GetOwnerGPT())
    {
        ASSERT(m_pGroupT != nullptr && m_pGroupT->GetMaxOccs() == 1);
    }

    bool SharesOwnerGroup(const VART* pVarT) const
    {
        return ( m_pGroupT == pVarT->GetOwnerGPT() );
    }

    bool IsApplicable() override
    {
        return ( m_pGroupT->GetTotalOccurrences() != 0 );
    }

private:
    const GROUPT* m_pGroupT;
};



// --------------------------------------------------------------------------
// a frequency applicability evaluator for multiply occurring items
// --------------------------------------------------------------------------

class MultiplyOccurringItemFrequencyApplicabilityEvaluator : public EngineFrequencyApplicabilityEvaluator
{
public:
    MultiplyOccurringItemFrequencyApplicabilityEvaluator(CIntDriver* pIntDriver, const VART* pVarT)
        :   m_pVarT(pVarT),
            m_pIntDriver(pIntDriver),
            m_iteratorMakeCalled(false),
            m_iteratorWasApplicable(false)
    {
        // use the old frequency evaluation code to determine the occurrences
        m_iterator.SetEngineDriver(m_pIntDriver->m_pEngineDriver);

        m_cMVarNode.m_iVarType = MVAR_CODE;
        m_cMVarNode.m_iVarIndex = pVarT->GetSymbolIndex();
        m_cMVarNode.m_iSubindexNumber = pVarT->GetNumDim();

        const GROUPT* pGroupT = pVarT->GetParentGPT();

        for( int iDim = pVarT->GetNumDim() - 1; iDim >= 0; --iDim )
        {
            // implicit index is calculated using the stack.
            // If it cannot be resolved by the stack, 1:totalocc is used
            m_cMVarNode.m_iVarSubindexType[iDim] = MVAR_GROUP;
            m_cMVarNode.m_iVarSubindexExpr[iDim] = pGroupT->GetSymbolIndex();

            pGroupT = pGroupT->GetParentGPT();
        }
    }

    bool SharesParentGroups(const VART* pVarT) const
    {
        if( m_pVarT->GetNumDim() != pVarT->GetNumDim() )
            return false;

        const GROUPT* this_pGroupT = m_pVarT->GetParentGPT();
        const GROUPT* compare_pGroupT = pVarT->GetParentGPT();

        for( int iDim = m_pVarT->GetNumDim() - 1; iDim >= 0; --iDim )
        {
            if( ( m_pVarT->GetDimType(iDim) != pVarT->GetDimType(iDim) ) || ( this_pGroupT != compare_pGroupT ) )
                return false;

            this_pGroupT = this_pGroupT->GetParentGPT();
            compare_pGroupT = compare_pGroupT->GetParentGPT();
        }

        return true;
    }

    bool IsApplicable() override
    {
        m_iteratorMakeCalled = false;
        return true;
    }

    bool IsIteratorApplicable()
    {
        if( !m_iteratorMakeCalled )
        {
            m_iteratorWasApplicable = m_iterator.Make(&m_cMVarNode, true);
            m_iteratorMakeCalled = true;
        }

        return m_iteratorWasApplicable;
    }

    std::tuple<size_t, size_t> GetRecordOccurrences() const
    {
        ASSERT(m_iteratorMakeCalled && m_iteratorWasApplicable);
        return std::make_tuple((size_t)m_iterator.GetLow(0), (size_t)m_iterator.GetHigh(0));
    }

    std::tuple<size_t, size_t> GetItemSubitemOccurrences(int dimension) const
    {
        ASSERT(m_iteratorMakeCalled && m_iteratorWasApplicable);
        ASSERT(m_iterator.GetLow(3 - dimension) == 0 && m_iterator.GetHigh(3 - dimension) == 0);

        return std::make_tuple((size_t)m_iterator.GetLow(dimension), (size_t)m_iterator.GetHigh(dimension));
    }


private:
    const VART* m_pVarT;
    CIntDriver* m_pIntDriver;
    MVAR_NODE m_cMVarNode;
    CIterator m_iterator;
    bool m_iteratorMakeCalled;
    bool m_iteratorWasApplicable;
};



// --------------------------------------------------------------------------
// the base frequency counter
// --------------------------------------------------------------------------

class EngineFrequencyEntry
{
public:
    virtual ~EngineFrequencyEntry() { }

    virtual size_t Tally(const double& weight) = 0;

    virtual void ClearTallies() = 0;

    virtual FrequencyCounter<double, double>& GetNumericSingleFrequencyCounter() { throw ProgrammingErrorException(); }
    virtual FrequencyCounter<std::wstring, double>& GetStringSingleFrequencyCounter() { throw ProgrammingErrorException(); }

    virtual std::vector<std::shared_ptr<FrequencyPrinterEntry<double, double>>> GetNumericFrequencyPrinterEntries() const { return { }; }
    virtual std::vector<std::shared_ptr<FrequencyPrinterEntry<std::wstring, double>>> GetStringFrequencyPrinterEntries() const { return { }; }
};



// --------------------------------------------------------------------------
// a processor for string variables to handle the breakdown string splitting
// --------------------------------------------------------------------------

namespace
{
    inline size_t TallyStringVariable(const std::wstring& value, const double& weight, const std::optional<int>& breakdown,
                                      FrequencyCounter<std::wstring, double>& frequency_counter)
    {
        if( !breakdown.has_value() )
        {
            frequency_counter.Add(value, weight);
            return 1;
        }

        else
        {
            wstring_view value_sv = value;
            size_t values_tallied = 0;

            while( !value_sv.empty() )
            {
                size_t this_value_length = std::min(value_sv.length(), static_cast<size_t>(*breakdown));

                frequency_counter.Add(value_sv.substr(0, this_value_length), weight);
                ++values_tallied;

                value_sv = value_sv.substr(this_value_length);
            }

            return values_tallied;
        }
    }
}



// --------------------------------------------------------------------------
// frequency counter for work variables
// --------------------------------------------------------------------------

class WorkVariableFrequencyEntry : public EngineFrequencyEntry
{
public:
    WorkVariableFrequencyEntry(const Logic::SymbolTable& symbol_table, int symbol_index)
        :   m_symbolTable(symbol_table),
            m_symbolIndex(symbol_index),
            m_frequencyCounter(FrequencyCounter<double, double>::Create())
    {
        ASSERT(NPT_Ref(m_symbolIndex).IsA(SymbolType::WorkVariable));
    }

    size_t Tally(const double& weight) override
    {
        const WorkVariable& work_variable = GetSymbolWorkVariable(m_symbolIndex);

        m_frequencyCounter->Add(work_variable.GetValue(), weight);

        return 1;
    }

    void ClearTallies() override
    {
        m_frequencyCounter->ClearCounts();
    }

    FrequencyCounter<double, double>& GetNumericSingleFrequencyCounter() override
    {
        return *m_frequencyCounter;
    }

    std::vector<std::shared_ptr<FrequencyPrinterEntry<double, double>>> GetNumericFrequencyPrinterEntries() const override
    {
        const WorkVariable& work_variable = GetSymbolWorkVariable(m_symbolIndex);

        return { std::make_shared<FrequencyPrinterEntry<double, double>>(m_frequencyCounter, work_variable.GetName()) };
    }

private:
    const Logic::SymbolTable& GetSymbolTable() const { return m_symbolTable; }

private:
    const Logic::SymbolTable& m_symbolTable;
    int m_symbolIndex;
    std::shared_ptr<FrequencyCounter<double, double>> m_frequencyCounter;
};



// --------------------------------------------------------------------------
// frequency counter for work strings
// --------------------------------------------------------------------------

class WorkStringFrequencyEntry : public EngineFrequencyEntry
{
public:
    WorkStringFrequencyEntry(const Logic::SymbolTable& symbol_table, int symbol_index, const FrequencyEntry& frequency_entry)
        :   m_symbolTable(symbol_table),
            m_symbolIndex(symbol_index),
            m_breakdown(frequency_entry.breakdown),
            m_frequencyCounter(FrequencyCounter<std::wstring, double>::Create())
    {
        ASSERT(NPT_Ref(m_symbolIndex).IsA(SymbolType::WorkString));
    };

    size_t Tally(const double& weight) override
    {
        const WorkString& work_string = GetSymbolWorkString(m_symbolIndex);

        return TallyStringVariable(work_string.GetString(), weight, m_breakdown, *m_frequencyCounter);
    }

    void ClearTallies() override
    {
        m_frequencyCounter->ClearCounts();
    }

    FrequencyCounter<std::wstring, double>& GetStringSingleFrequencyCounter() override
    {
        return *m_frequencyCounter;
    }

    std::vector<std::shared_ptr<FrequencyPrinterEntry<std::wstring, double>>> GetStringFrequencyPrinterEntries() const override
    {
        const WorkString& work_string = GetSymbolWorkString(m_symbolIndex);

        return { std::make_shared<FrequencyPrinterEntry<std::wstring, double>>(m_frequencyCounter, work_string.GetName()) };
    }

private:
    const Logic::SymbolTable& GetSymbolTable() const { return m_symbolTable; }

private:
    const Logic::SymbolTable& m_symbolTable;
    int m_symbolIndex;
    std::optional<int> m_breakdown;
    std::shared_ptr<FrequencyCounter<std::wstring, double>> m_frequencyCounter;
};



// --------------------------------------------------------------------------
// frequency counter for a singly occurring numeric variable
// --------------------------------------------------------------------------

class SinglyOccurringNumericVariableFrequencyEntry : public EngineFrequencyEntry
{
public:
    SinglyOccurringNumericVariableFrequencyEntry(CIntDriver* pIntDriver, const VART* pVarT)
        :   m_pVarT(pVarT),
            m_frequencyCounter(FrequencyCounter<double, double>::Create(pVarT->GetDictItem()))
    {
        m_variableBuffer = pIntDriver->GetSingVarFloatAddr(const_cast<VART*>(m_pVarT));
    };

    size_t Tally(const double& weight) override
    {
        m_frequencyCounter->Add(*m_variableBuffer, weight);
        return 1;
    }

    void ClearTallies() override
    {
        m_frequencyCounter->ClearCounts();
    }

    FrequencyCounter<double, double>& GetNumericSingleFrequencyCounter() override
    {
        return *m_frequencyCounter;
    }

    std::vector<std::shared_ptr<FrequencyPrinterEntry<double, double>>> GetNumericFrequencyPrinterEntries() const override
    {
        return { std::make_shared<FrequencyPrinterEntry<double, double>>(m_frequencyCounter, *m_pVarT->GetDictItem(),
                                                                         m_pVarT->GetCurrentDictValueSet(), std::nullopt, std::nullopt) };
    }

private:
    const VART* m_pVarT;
    const double* m_variableBuffer;
    std::shared_ptr<FrequencyCounter<double, double>> m_frequencyCounter;
};



// --------------------------------------------------------------------------
// frequency counter for a singly occurring string variable
// --------------------------------------------------------------------------

class SinglyOccurringStringVariableFrequencyEntry : public EngineFrequencyEntry
{
public:
    SinglyOccurringStringVariableFrequencyEntry(CIntDriver* pIntDriver, const VART* pVarT, const FrequencyEntry& frequency_entry)
        :   m_pVarT(pVarT),
            m_breakdown(frequency_entry.breakdown),
            m_frequencyCounter(FrequencyCounter<std::wstring, double>::Create(pVarT->GetDictItem()))
    {
        m_variableBuffer = pIntDriver->GetSingVarAsciiAddr(const_cast<VART*>(m_pVarT));
    };

    size_t Tally(const double& weight) override
    {
        return TallyStringVariable(std::wstring(m_variableBuffer, m_pVarT->GetLength()), weight, m_breakdown, *m_frequencyCounter);
    }

    void ClearTallies() override
    {
        m_frequencyCounter->ClearCounts();
    }

    FrequencyCounter<std::wstring, double>& GetStringSingleFrequencyCounter() override
    {
        return *m_frequencyCounter;
    }

    std::vector<std::shared_ptr<FrequencyPrinterEntry<std::wstring, double>>> GetStringFrequencyPrinterEntries() const override
    {
        return { std::make_shared<FrequencyPrinterEntry<std::wstring, double>>(m_frequencyCounter, *m_pVarT->GetDictItem(),
                                                                               m_pVarT->GetCurrentDictValueSet(), std::nullopt, std::nullopt) };
    }

private:
    const VART* m_pVarT;
    const TCHAR* m_variableBuffer;
    std::optional<int> m_breakdown;
    std::shared_ptr<FrequencyCounter<std::wstring, double>> m_frequencyCounter;
};



// --------------------------------------------------------------------------
// frequency counter for multiply occurring numeric and string variables
// --------------------------------------------------------------------------

template<typename T>
class MultiplyOccurringVariableFrequencyEntry : public EngineFrequencyEntry
{
public:
    MultiplyOccurringVariableFrequencyEntry(CIntDriver* pIntDriver, const VART* pVarT, const FrequencyEntry& frequency_entry,
        MultiplyOccurringItemFrequencyApplicabilityEvaluator& multiply_occurring_item_evaluator)
        :   m_pIntDriver(pIntDriver),
            m_pVarT(pVarT),
            m_frequencyEntry(frequency_entry),
            m_multiplyOccurringItemEvaluator(multiply_occurring_item_evaluator),
            m_theIndex(ZERO_BASED)
    {
        m_theIndex.setAtOrigin();

        if( m_pVarT->GetDictItem()->GetItemSubitemOccurs() > 1 )
        {
            ASSERT(m_pVarT->GetNumDim() <= 2);

            // a repeating item (or a subitem on a repeating item)
            if( m_pVarT->GetDimType(0) == CDimension::VDimType::Item || m_pVarT->GetDimType(1) == CDimension::VDimType::Item )
            {
                m_itemSubitemOccurrencesIndex = 1;
            }

            // a repeating subitem
            else
            {
                ASSERT(m_pVarT->GetDimType(0) == CDimension::VDimType::SubItem || m_pVarT->GetDimType(1) == CDimension::VDimType::SubItem);
                m_itemSubitemOccurrencesIndex = 2;
            }
        }

        // setup each of the frequency counters
        ASSERT(!m_frequencyEntry.occurrence_details.empty());

        SetupDefaultFrequencyCounters();
    };

    size_t Tally(const double& weight) override
    {
        size_t values_tallied = 0;

        for( size_t occurrence_details_index = 0; occurrence_details_index < m_frequencyEntry.occurrence_details.size(); ++occurrence_details_index )
        {
            const auto& occurrence_details = m_frequencyEntry.occurrence_details[occurrence_details_index];

            // if an implicit record occurrence is used, we need to evaluate the valid
            // occurrences and potentially skip this entry
            if( occurrence_details.combine_record_occurrences )
            {
               if( !m_multiplyOccurringItemEvaluator.IsIteratorApplicable() )
                    continue;

                const auto& record_occurrences = m_multiplyOccurringItemEvaluator.GetRecordOccurrences();

                for( size_t record_occurrence = std::get<0>(record_occurrences); record_occurrence <= std::get<1>(record_occurrences); ++record_occurrence )
                    TallyRecordOccurrence(weight, occurrence_details_index, record_occurrence, values_tallied);
            }

            // otherwise process each explicit record occurrence
            else
            {
                static_assert(FrequencySetting::IncludeAllRecordOccurrencesWhenDisjoint);
                ASSERT(!occurrence_details.record_occurrences_to_explicitly_display.empty());

                for( size_t record_occurrence : occurrence_details.record_occurrences_to_explicitly_display )
                    TallyRecordOccurrence(weight, occurrence_details_index, record_occurrence, values_tallied);
            }
        }

        return values_tallied;
    }

    private:
        void TallyRecordOccurrence(const double& weight, size_t occurrence_details_index, size_t record_occurrence, size_t& values_tallied)
        {
            const auto& occurrence_details = m_frequencyEntry.occurrence_details[occurrence_details_index];
            auto& frequency_counters = m_perOccurrenceFrequencyCounters[occurrence_details_index];
            size_t frequency_counter_index = occurrence_details.combine_record_occurrences ? 0 : record_occurrence;

            ASSERT(frequency_counter_index < frequency_counters.size());
            auto& frequency_counter = frequency_counters[frequency_counter_index];
            ASSERT(frequency_counter != nullptr);

            m_theIndex.setIndexValue(0, (int)record_occurrence);

            auto execute_tally = [&]
            {
                if constexpr(std::is_same_v<T, double>)
                {
                    T value = m_pIntDriver->GetMultVarFloatValue(const_cast<VART*>(m_pVarT), m_theIndex);
                    frequency_counter->Add(value, weight);
                    ++values_tallied;
                }

                else
                {
                    T value(m_pIntDriver->GetMultVarAsciiAddr(const_cast<VART*>(m_pVarT), m_theIndex), m_pVarT->GetLength());
                    values_tallied += TallyStringVariable(value, weight, m_frequencyEntry.breakdown, *frequency_counter);
                }
            };

            // if there is no item/subitem occurrence, simply execute the tally
            if( !m_itemSubitemOccurrencesIndex.has_value() )
            {
                execute_tally();
            }

            // otherwise see if this entry should be skipped
            else
            {
                // if a specific item/subitem occurrence was specified, it will always be evaluated
                if( occurrence_details.min_item_subitem_occurrence == occurrence_details.max_item_subitem_occurrence )
                {
                    m_theIndex.setIndexValue(*m_itemSubitemOccurrencesIndex, (int)occurrence_details.min_item_subitem_occurrence);
                    execute_tally();
                }

                // otherwise evaluate the valid item/subitem occurrences
                else if( m_multiplyOccurringItemEvaluator.IsIteratorApplicable() )
                {
                    std::tuple<size_t, size_t> item_subitem_occurrences = m_multiplyOccurringItemEvaluator.GetItemSubitemOccurrences(*m_itemSubitemOccurrencesIndex);

                    for( size_t item_subitem_occurrence = std::get<0>(item_subitem_occurrences); item_subitem_occurrence <= std::get<1>(item_subitem_occurrences); ++item_subitem_occurrence )
                    {
                        m_theIndex.setIndexValue(*m_itemSubitemOccurrencesIndex, (int)item_subitem_occurrence);
                        execute_tally();
                    }
                }
            }
        }

public:
    void ClearTallies() override
    {
        SetupDefaultFrequencyCounters();
    }

    FrequencyCounter<double, double>& GetNumericSingleFrequencyCounter() override
    {
        if constexpr(std::is_same_v<T, double>)
        {
            return GetSingleFrequencyCounter();
        }

        else
        {
            throw ProgrammingErrorException();
        }
    }

    FrequencyCounter<std::wstring, double>& GetStringSingleFrequencyCounter() override
    {
        if constexpr(std::is_same_v<T, std::wstring>)
        {
            return GetSingleFrequencyCounter();
        }

        else
        {
            throw ProgrammingErrorException();
        }
    }

    std::vector<std::shared_ptr<FrequencyPrinterEntry<double, double>>> GetNumericFrequencyPrinterEntries() const override
    {
        if constexpr(std::is_same_v<T, double>)
        {
            return GetFrequencyPrinterEntries();
        }

        else
        {
            return { };
        }
    }

    std::vector<std::shared_ptr<FrequencyPrinterEntry<std::wstring, double>>> GetStringFrequencyPrinterEntries() const override
    {
        if constexpr(std::is_same_v<T, std::wstring>)
        {
            return GetFrequencyPrinterEntries();
        }

        else
        {
            return { };
        }
    }

private:
    void SetupDefaultFrequencyCounters()
    {
        m_perOccurrenceFrequencyCounters.clear();

        for( const auto& occurrence_details : m_frequencyEntry.occurrence_details )
        {
            auto& frequency_counters = m_perOccurrenceFrequencyCounters.emplace_back();

            // add frequency counters for any specific record occurrences requested
            for( const auto& record_occurrence : occurrence_details.record_occurrences_to_explicitly_display )
            {
                ASSERT(!occurrence_details.combine_record_occurrences);
                EnsureFrequencyCounterExists(frequency_counters, record_occurrence);
            }

            // if none have been created, then ensure there is at least one (for combined record occurrences
            // or as a placeholder for the first disjoint record occurrence)
            if( frequency_counters.empty() )
            {
                ASSERT(occurrence_details.combine_record_occurrences || occurrence_details.disjoint_record_occurrences);
                EnsureFrequencyCounterExists(frequency_counters, 0);
            }
        }
    }

    void EnsureFrequencyCounterExists(std::vector<std::shared_ptr<FrequencyCounter<T, double>>>& frequency_counters, size_t index)
    {
        if( index >= frequency_counters.size() )
            frequency_counters.resize(index + 1);

        if( frequency_counters[index] == nullptr )
            frequency_counters[index] = FrequencyCounter<T, double>::Create(m_pVarT->GetDictItem());
    }

    FrequencyCounter<T, double>& GetSingleFrequencyCounter()
    {
        // we should only be here is there really is only a single frequency counter
        ASSERT(m_perOccurrenceFrequencyCounters.size() == 1 && m_perOccurrenceFrequencyCounters.front().size() == 1);

        return *m_perOccurrenceFrequencyCounters.front().front();
    }

    std::vector<std::shared_ptr<FrequencyPrinterEntry<T, double>>> GetFrequencyPrinterEntries() const
    {
        std::vector<std::shared_ptr<FrequencyPrinterEntry<T, double>>> frequency_printer_entries;

        // add all used frequency counters
        auto frequency_counters_itr = m_perOccurrenceFrequencyCounters.begin();

        for( const auto& occurrence_details : m_frequencyEntry.occurrence_details )
        {
            for( size_t record_occurrence = 0; record_occurrence < frequency_counters_itr->size(); ++record_occurrence )
            {
                auto& frequency_counter = (*frequency_counters_itr)[record_occurrence];

                if( frequency_counter != nullptr )
                {
                    // only set the record and item/subitem occurrences when they are not combined
                    std::optional<size_t> record_occurrence_for_fpe;
                    std::optional<size_t> item_subitem_occurrence_for_fpe;

                    if( !occurrence_details.combine_record_occurrences )
                        record_occurrence_for_fpe = record_occurrence;

                    if( m_pVarT->GetDictItem()->GetItemSubitemOccurs() > 1 &&
                        occurrence_details.min_item_subitem_occurrence == occurrence_details.max_item_subitem_occurrence )
                    {
                        item_subitem_occurrence_for_fpe = occurrence_details.min_item_subitem_occurrence;
                    }

                    frequency_printer_entries.emplace_back(std::make_shared<FrequencyPrinterEntry<T, double>>(frequency_counter,
                        *m_pVarT->GetDictItem(), m_pVarT->GetCurrentDictValueSet(), record_occurrence_for_fpe, item_subitem_occurrence_for_fpe));
                }
            }

            ++frequency_counters_itr;
        }

        // sort the frequencies so that item/subitem occurrences fall within their record occurrence
        std::sort(frequency_printer_entries.begin(), frequency_printer_entries.end(),
            [](const auto& fpe1, const auto& fpe2) { return fpe1->Compare(*fpe2); });

        return frequency_printer_entries;
    }

private:
    CIntDriver* m_pIntDriver;
    const VART* m_pVarT;
    const FrequencyEntry& m_frequencyEntry;
    MultiplyOccurringItemFrequencyApplicabilityEvaluator& m_multiplyOccurringItemEvaluator;
    CNDIndexes m_theIndex;
    std::optional<int> m_itemSubitemOccurrencesIndex;
    std::vector<std::vector<std::shared_ptr<FrequencyCounter<T, double>>>> m_perOccurrenceFrequencyCounters;
};



// --------------------------------------------------------------------------
// the frequency driver implementation
// --------------------------------------------------------------------------

FrequencyDriver::FrequencyDriver(CIntDriver& int_driver)
    :   m_pEngineArea(int_driver.m_pEngineArea),
        m_pEngineDriver(int_driver.m_pEngineDriver),
        m_pIntDriver(&int_driver),
        m_engineData(&m_pEngineArea->GetEngineData()),
        m_symbolTable(int_driver.GetSymbolTable())
{
    // setup the frequencies
    for( const Frequency& frequency : VI_V(m_engineData->frequencies) )
        CreateEngineFrequencyEntries(frequency);

    // setup the frequency printer
    try
    {
        if( !m_pEngineDriver->GetPifFile()->GetFrequenciesFilename().IsEmpty() )
            m_frequencyPrinter = CreateFrequencyPrinter(CS2WS(m_pEngineDriver->GetPifFile()->GetFrequenciesFilename()));
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 94531, exception.GetErrorMessage().c_str());
    }
}


FrequencyDriver::~FrequencyDriver()
{
    // write unnamed frequencies
    try
    {
        std::shared_ptr<FrequencyPrinter> frequency_printer;

        for( size_t frequency_index = 0; frequency_index < m_engineData->frequencies.size(); ++frequency_index )
        {
            const Frequency& frequency = *m_engineData->frequencies[frequency_index];

            if( !frequency.GetNamedFrequencySymbolIndex().has_value() )
            {
                if( frequency_printer == nullptr )
                    frequency_printer = GetDefaultFrequencyPrinter();

                PrintFrequencies(frequency_index, *frequency_printer, std::wstring(), frequency.GetFrequencyPrinterOptions());
            }
        }
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 94531, exception.GetErrorMessage().c_str());
    }
}


std::unique_ptr<FrequencyPrinter> FrequencyDriver::CreateFrequencyPrinter(std::wstring filename, const PFF& pff)
{
    ASSERT(!filename.empty());
    std::wstring extension = PortableFunctions::PathGetFileExtension(filename);

    if( SO::EqualsNoCase(extension, FileExtensions::Table) )
    {
#ifdef WIN_DESKTOP
        return std::make_unique<TableFrequencyPrinter>(std::move(filename));
#else
        throw CSProException("You must use CSPro on a Windows desktop to write frequencies to a table (.tbw) file.");
#endif
    }

    else if( SO::EqualsOneOfNoCase(extension, FileExtensions::HTML, FileExtensions::HTM) )
    {
        return std::make_unique<HtmlFrequencyPrinter>(filename);
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::Excel) )
    {
        return std::make_unique<ExcelFrequencyPrinter>(filename);
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::Json) )
    {
        return std::make_unique<JsonFileFrequencyPrinter>(filename);
    }

    else
    {
        return std::make_unique<TextFileFrequencyPrinter>(filename, pff.GetListingWidth());
    }
}


std::unique_ptr<FrequencyPrinter> FrequencyDriver::CreateFrequencyPrinter(std::wstring filename)
{
    return CreateFrequencyPrinter(std::move(filename), *m_pEngineDriver->GetPifFile());
}


std::shared_ptr<FrequencyPrinter> FrequencyDriver::GetDefaultFrequencyPrinter()
{
    if( m_frequencyPrinter != nullptr )
        return m_frequencyPrinter;

    // if a frequency printer is needed and has not been created based on a file, try to write to the listing file
    std::optional<std::tuple<Listing::ListingType, void*>> features = m_pEngineDriver->GetLister()->GetFrequencyPrinterFeatures();

    if( features.has_value() )
    {
        ASSERT(std::get<1>(*features) != nullptr);

        if( std::get<0>(*features) == Listing::ListingType::Text )
        {
            CStdioFileUnicode* file = static_cast<CStdioFileUnicode*>(std::get<1>(*features));
            return std::make_shared<TextFileFrequencyPrinter>(*file, m_pEngineDriver->GetPifFile()->GetListingWidth());
        }

        else if( std::get<0>(*features) == Listing::ListingType::Html )
        {
            CStdioFileUnicode* file = static_cast<CStdioFileUnicode*>(std::get<1>(*features));
            return std::make_shared<HtmlFrequencyPrinter>(*file);
        }

        else if( std::get<0>(*features) == Listing::ListingType::Json )
        {
            std::string* json_frequency_text = static_cast<std::string*>(std::get<1>(*features));
            return std::make_shared<JsonStringFrequencyPrinter>(*json_frequency_text);
        }

        else
        {
            ASSERT(false);
        }
    }

    throw CSProException("The listing file does not support the writing of frequencies.");
}


void FrequencyDriver::CreateEngineFrequencyEntries(const Frequency& frequency)
{
    // link the frequency driver in case the named frequencies object is cloned
    if( frequency.GetNamedFrequencySymbolIndex().has_value() )
    {
        NamedFrequency& named_frequency = GetSymbolLogicNamedFrequency(*frequency.GetNamedFrequencySymbolIndex());
        named_frequency.SetFrequencyDriver(this);
    }

    // setup the frequency calculation variables
    auto& engine_frequency_entries = m_engineFrequencyEntries.emplace_back();
    auto& engine_frequency_applicability_evaluators = m_engineFrequencyApplicabilityEvaluators.emplace_back();

    std::shared_ptr<AlwaysApplicableEngineFrequencyApplicabilityEvaluator> always_applicable_evaluator;
    std::vector<std::shared_ptr<SinglyOccurringItemFrequencyApplicabilityEvaluator>> singly_occurring_item_evaluators;
    std::vector<std::shared_ptr<MultiplyOccurringItemFrequencyApplicabilityEvaluator>> multiply_occurring_item_evaluators;

    auto get_always_applicable_evaluator = [&]
    {
        if( always_applicable_evaluator == nullptr )
        {
            always_applicable_evaluator = std::make_shared<AlwaysApplicableEngineFrequencyApplicabilityEvaluator>();
            engine_frequency_applicability_evaluators.emplace_back(always_applicable_evaluator);
        }

        return always_applicable_evaluator;
    };


    auto get_singly_occurring_item_evaluator = [&](const VART* pVarT)
    {
        // see if one exists that can be reused
        for( auto& singly_occurring_item_evaluator : singly_occurring_item_evaluators )
        {
            if( singly_occurring_item_evaluator->SharesOwnerGroup(pVarT) )
                return singly_occurring_item_evaluator;
        }

        // otherwise create a new one
        auto singly_occurring_item_evaluator = std::make_shared<SinglyOccurringItemFrequencyApplicabilityEvaluator>(pVarT);

        singly_occurring_item_evaluators.emplace_back(singly_occurring_item_evaluator);
        engine_frequency_applicability_evaluators.emplace_back(singly_occurring_item_evaluator);

        return singly_occurring_item_evaluator;
    };


    auto get_multiply_occurring_item_evaluator = [&](const VART* pVarT)
    {
        // see if one exists that can be reused
        for( auto& multiply_occurring_item_evaluator : multiply_occurring_item_evaluators )
        {
            if( multiply_occurring_item_evaluator->SharesParentGroups(pVarT) )
                return multiply_occurring_item_evaluator;
        }

        // otherwise create a new one
        auto multiply_occurring_item_evaluator = std::make_shared<MultiplyOccurringItemFrequencyApplicabilityEvaluator>(m_pIntDriver, pVarT);

        multiply_occurring_item_evaluators.emplace_back(multiply_occurring_item_evaluator);
        engine_frequency_applicability_evaluators.emplace_back(multiply_occurring_item_evaluator);

        return multiply_occurring_item_evaluator;
    };


    // set up each frequency evaluator and counter
    for( const FrequencyEntry& frequency_entry : frequency.GetFrequencyEntries() )
    {
        std::shared_ptr<EngineFrequencyApplicabilityEvaluator> engine_frequency_applicability_evaluator;
        std::shared_ptr<EngineFrequencyEntry> engine_frequency_entry;

        const Symbol& symbol = NPT_Ref(frequency_entry.symbol_index);

        // a work variable
        if( symbol.IsA(SymbolType::WorkVariable) )
        {
            engine_frequency_applicability_evaluator = get_always_applicable_evaluator();

            engine_frequency_entry = std::make_shared<WorkVariableFrequencyEntry>(GetSymbolTable(), frequency_entry.symbol_index);
        }

        // a work string
        else if( symbol.IsA(SymbolType::WorkString) )
        {
            engine_frequency_applicability_evaluator = get_always_applicable_evaluator();

            engine_frequency_entry = std::make_shared<WorkStringFrequencyEntry>(GetSymbolTable(), frequency_entry.symbol_index, frequency_entry);
        }

        else
        {
            ASSERT(symbol.IsA(SymbolType::Variable));
            const VART* pVarT = assert_cast<const VART*>(&symbol);

            // singly occurring items on singly occurring records
            if( !pVarT->IsArray() )
            {
                ASSERT(frequency_entry.occurrence_details.size() == 1 &&
                       frequency_entry.occurrence_details.front().min_item_subitem_occurrence == 0 &&
                       frequency_entry.occurrence_details.front().max_item_subitem_occurrence == 0);

                engine_frequency_applicability_evaluator = get_singly_occurring_item_evaluator(pVarT);

                if( pVarT->IsNumeric() )
                {
                    engine_frequency_entry = std::make_shared<SinglyOccurringNumericVariableFrequencyEntry>(m_pIntDriver, pVarT);
                }

                else
                {
                    engine_frequency_entry = std::make_shared<SinglyOccurringStringVariableFrequencyEntry>(m_pIntDriver, pVarT, frequency_entry);
                }
            }

            // an item that repeats
            else
            {
                auto multiply_occurring_item_evaluator = get_multiply_occurring_item_evaluator(pVarT);
                engine_frequency_applicability_evaluator = multiply_occurring_item_evaluator;

                if( pVarT->IsNumeric() )
                {
                    engine_frequency_entry = std::make_shared<MultiplyOccurringVariableFrequencyEntry<double>>(
                        m_pIntDriver, pVarT, frequency_entry, *multiply_occurring_item_evaluator);
                }

                else
                {
                    engine_frequency_entry = std::make_shared<MultiplyOccurringVariableFrequencyEntry<std::wstring>>(
                        m_pIntDriver, pVarT, frequency_entry, *multiply_occurring_item_evaluator);
                }
            }
        }

        ASSERT(engine_frequency_applicability_evaluator != nullptr && engine_frequency_entry != nullptr);

        engine_frequency_applicability_evaluator->AddEngineFrequencyEntry(engine_frequency_entry);
        engine_frequency_entries.emplace_back(engine_frequency_entry);
    }
}


void FrequencyDriver::CloneFrequencyInInitialState(NamedFrequency& cloned_named_frequency, size_t source_frequency_index)
{
    // create a copy of the frequency, linking it to the cloned named frequency
    cloned_named_frequency.SetFrequencyIndex(m_engineData->frequencies.size());

    Frequency& cloned_frequency = *m_engineData->frequencies.emplace_back(
        std::make_shared<Frequency>(*m_engineData->frequencies[source_frequency_index]));

    CreateEngineFrequencyEntries(cloned_frequency);
}


void FrequencyDriver::ResetFrequency(size_t frequency_index, int heading_expressions_list_node_index)
{
    // clear the tallies
    ClearFrequencyTallies(frequency_index);

    // and reset the headings (if they were not string literals)
    if( heading_expressions_list_node_index != -1 )
    {
        Frequency& frequency = *m_engineData->frequencies[frequency_index];
        frequency.GetFrequencyPrinterOptions().SetHeadings(EvaluateDynamicHeadings(m_pIntDriver, heading_expressions_list_node_index));
    }
}


void FrequencyDriver::ClearFrequencyTallies(size_t frequency_index)
{
    for( EngineFrequencyEntry& engine_frequency_entry : VI_V(m_engineFrequencyEntries[frequency_index]) )
        engine_frequency_entry.ClearTallies();
}


double FrequencyDriver::TallyFrequency(size_t frequency_index, int weight_expression)
{
    double weight;

    if( weight_expression == -1 )
    {
        weight = 1;
    }

    else
    {
        weight = m_pIntDriver->evalexpr(weight_expression);

        // dont't tally invalid weights
        if( IsSpecial(weight) )
        {
            const Frequency& frequency = *m_engineData->frequencies[frequency_index];

            issaerror(MessageType::Error, 94532, frequency.GetNamedFrequencySymbolIndex().has_value() ?
                      NPT_Ref(*frequency.GetNamedFrequencySymbolIndex()).GetName().c_str() : _T("<unnamed>"));

            return DEFAULT;
        }
    }

    size_t values_tallied = 0;

    for( EngineFrequencyApplicabilityEvaluator& engine_frequency_applicability_evaluator : VI_V(m_engineFrequencyApplicabilityEvaluators[frequency_index]) )
    {
        if( engine_frequency_applicability_evaluator.IsApplicable() )
        {
            for( EngineFrequencyEntry& engine_frequency_entry : engine_frequency_applicability_evaluator.GetEngineFrequencyEntriesIterator() )
                values_tallied += engine_frequency_entry.Tally(weight);
        }
    }

    return values_tallied;
}


void FrequencyDriver::PrintFrequencies(size_t frequency_index, FrequencyPrinter& frequency_printer,
                                       std::wstring frequency_name, const FrequencyPrinterOptions& frequency_printer_options)
{
    frequency_printer.StartFrequencyGroup();

    for( const EngineFrequencyEntry& engine_frequency_entry : VI_V(m_engineFrequencyEntries[frequency_index]) )
    {
        auto print = [&](const auto& frequency_printer_entries)
        {
            for( auto& frequency_printer_entry : frequency_printer_entries )
                frequency_printer_entry->Print(frequency_printer, frequency_name, frequency_printer_options);
        };

        print(engine_frequency_entry.GetNumericFrequencyPrinterEntries());
        print(engine_frequency_entry.GetStringFrequencyPrinterEntries());
    }
}


template<typename GetCallback, typename SetCallback>
void FrequencyDriver::UseSingleFrequencyCounterForGettingAndSetting(int var_node_index, GetCallback get_callback, SetCallback set_callback)
{
    const auto& element_reference_node = m_pIntDriver->GetNode<Nodes::ElementReference>(var_node_index);
    const NamedFrequency& named_frequency = GetSymbolLogicNamedFrequency(element_reference_node.symbol_index);

    Frequency& frequency = *m_engineData->frequencies[named_frequency.GetFrequencyIndex()];
    ASSERT(frequency.GetSymbolIndexOfSingleFrequencyVariable().has_value());

    auto& engine_frequency_entry = m_engineFrequencyEntries[named_frequency.GetFrequencyIndex()];
    ASSERT(engine_frequency_entry.size() == 1);

    auto do_get_set = [&](auto& frequency_counter, const auto& value)
    {
        if constexpr(!std::is_same_v<GetCallback, bool>)
            get_callback(frequency_counter.GetCount(value));

        if constexpr(!std::is_same_v<SetCallback, bool>)
            frequency_counter.SetCount(value, set_callback());
    };

    const Symbol& symbol = NPT_Ref(*frequency.GetSymbolIndexOfSingleFrequencyVariable());

    if( IsNumeric(symbol) )
    {
        do_get_set(engine_frequency_entry.front()->GetNumericSingleFrequencyCounter(),
                   m_pIntDriver->evalexpr(element_reference_node.element_expressions[0]));
    }

    else
    {
        ASSERT(IsString(symbol));

        std::wstring value = m_pIntDriver->EvalAlphaExpr(element_reference_node.element_expressions[0]);

        // add spacing to fill out the string (if necessary)
        if( symbol.IsA(SymbolType::Variable) )
        {
            const VART* pVarT = assert_cast<const VART*>(&symbol);
            SO::MakeExactLength(value, pVarT->GetLength());
        }

        else if( symbol.GetSubType() == SymbolSubType::WorkAlpha )
        {
            const WorkAlpha& work_alpha = assert_cast<const WorkAlpha&>(symbol);
            SO::MakeExactLength(value, work_alpha.GetLength());
        }

        do_get_set(engine_frequency_entry.front()->GetStringSingleFrequencyCounter(), value);
    }
}


double FrequencyDriver::GetSingleFrequencyCounterCount(int var_node_index)
{
    double count;
    UseSingleFrequencyCounterForGettingAndSetting(var_node_index,
        [&](double c) { count = c; },
        false);
    return count;
}


void FrequencyDriver::SetSingleFrequencyCounterCount(int var_node_index, double count)
{
    UseSingleFrequencyCounterForGettingAndSetting(var_node_index,
        false,
        [&] { return count; });
}


void FrequencyDriver::ModifySingleFrequencyCounterCount(int var_node_index, const std::function<void(double&)>& modify_count_function)
{
    double count;
    UseSingleFrequencyCounterForGettingAndSetting(var_node_index,
        [&](double c) { count = c; modify_count_function(count); },
        [&] { return count; });
}


void FrequencyDriver::WriteJsonMetadata_subclass(const NamedFrequency& named_frequency, JsonWriter& json_writer) const
{
    ASSERT(!named_frequency.IsFunctionParameter());
    const Frequency& frequency = *m_engineData->frequencies[named_frequency.GetFrequencyIndex()];

    // the frequency inputs
    json_writer.BeginArray(JK::inputs);

    for( const FrequencyEntry& frequency_entry : frequency.GetFrequencyEntries() )
    {
        const Symbol& symbol = NPT_Ref(frequency_entry.symbol_index);

        json_writer.BeginObject();

        json_writer.Key(JK::symbol);
        symbol.WriteJson(json_writer, Symbol::SymbolJsonOutput::Metadata);

        json_writer.EndObject();
    }

    json_writer.EndArray();

    // the frequency formatting options
    json_writer.WriteObject(JK::formatOptions, frequency.GetFrequencyPrinterOptions(),
        [&](const FrequencyPrinterOptions& frequency_printer_options)
        {
            json_writer.Write(JK::heading, frequency_printer_options.GetHeadings())
                       .Write(JK::distinct, frequency_printer_options.GetDistinct());

            json_writer.WriteArray(JK::valueSets, frequency_printer_options.GetValueSetSymbolIndices(),
                [&](int symbol_index)
                {
                    json_writer.Write(NPT_Ref(symbol_index).GetName());
                });

            json_writer.Write(JK::useAllValueSets, frequency_printer_options.GetUseAllValueSets())
                       .Write(JK::statistics, frequency_printer_options.GetShowStatistics())
                       .Write(JK::frequencies, !frequency_printer_options.GetShowNoFrequencies());

            if( frequency_printer_options.GetUsingPercentiles() )
                json_writer.Write(JK::percentiles, frequency_printer_options.GetPercentiles());

            json_writer.Write(JK::netPercent, frequency_printer_options.GetShowNetPercents());

            if( frequency_printer_options.GetUsingDecimals() )
                json_writer.Write(JK::decimals, frequency_printer_options.GetDecimals());

            if( frequency_printer_options.GetUsingPageLength() )
                json_writer.Write(JK::pageLength, frequency_printer_options.GetPageLength());

            json_writer.BeginObject(JK::sort)
                       .Write(JK::ascending, frequency_printer_options.IsAscendingSort())
                       .Write(JK::order, frequency_printer_options.GetSortType())
                       .EndObject();
        });
}


void FrequencyDriver::WriteValueToJson(const NamedFrequency& named_frequency, JsonWriter& json_writer)
{
    ASSERT(!named_frequency.IsFunctionParameter());
    const Frequency& frequency = *m_engineData->frequencies[named_frequency.GetFrequencyIndex()];

    JsonFrequencyPrinter json_frequency_printer(json_writer);

    json_writer.BeginArray();
    PrintFrequencies(named_frequency.GetFrequencyIndex(), json_frequency_printer, named_frequency.GetName(), frequency.GetFrequencyPrinterOptions());
    json_writer.EndArray();
}



// --------------------------------------------------------------------------
// the frequency functions
// --------------------------------------------------------------------------

double CIntDriver::exfrequnnamed(int iExpr)
{
    const auto& unnamed_frequency_node = GetNode<Nodes::UnnamedFrequency>(iExpr);

    // evaluate any dynamic headings
    if( unnamed_frequency_node.heading_expressions_list_node != -1 )
    {
        Frequency& frequency = *m_engineData->frequencies[unnamed_frequency_node.frequency_index];
        frequency.GetFrequencyPrinterOptions().SetHeadings(EvaluateDynamicHeadings(this, unnamed_frequency_node.heading_expressions_list_node));
    }

    // quit out if the universe condition is not met
    if( unnamed_frequency_node.universe_expression != -1 && ConditionalValueIsFalse(evalexpr(unnamed_frequency_node.universe_expression)) )
        return 0;

    return m_frequencyDriver->TallyFrequency(unnamed_frequency_node.frequency_index, unnamed_frequency_node.weight_expression);
}


double CIntDriver::exfreqclear(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const NamedFrequency& named_frequency = GetSymbolLogicNamedFrequency(symbol_va_node.symbol_index);

    m_frequencyDriver->ClearFrequencyTallies(named_frequency.GetFrequencyIndex());

    return 1;
}


double CIntDriver::exfreqtally(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const NamedFrequency& named_frequency = GetSymbolLogicNamedFrequency(symbol_va_node.symbol_index);

    return m_frequencyDriver->TallyFrequency(named_frequency.GetFrequencyIndex(), symbol_va_node.arguments[0]);
}


double CIntDriver::exfreqsave(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const NamedFrequency& named_frequency = GetSymbolLogicNamedFrequency(symbol_va_node.symbol_index);
    const Frequency& frequency = *m_engineData->frequencies[named_frequency.GetFrequencyIndex()];

    const int* arguments = symbol_va_node.arguments;
    std::unique_ptr<int[]> pre80_arguments;

    if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
    {
        pre80_arguments = std::make_unique<int[]>(3);
        pre80_arguments[0] = -1;
        pre80_arguments[1] = arguments[0];
        pre80_arguments[2] = arguments[1];
        arguments = pre80_arguments.get();
    }

    try
    {
        // the frequency can be saved to ...
        std::shared_ptr<FrequencyPrinter> frequency_printer;
        std::unique_ptr<std::tuple<HtmlStringWriter, std::wstring*>> html_writer_and_report_text_builder;

        // ...a file
        if( arguments[0] == -1 && arguments[1] != -1 )
        {
            std::wstring filename = EvalFullPathFileName(arguments[1]);
            frequency_printer = m_frequencyDriver->CreateFrequencyPrinter(std::move(filename));
        }

        // ... a report
        else if( arguments[0] == (int)SymbolType::Report )
        {
            Report& report = GetSymbolReport(arguments[1]);

            if( !report.IsHtmlType() )
            {
                issaerror(MessageType::Error, 94533, named_frequency.GetName().c_str(), report.GetName().c_str());
                return 0;
            }

            html_writer_and_report_text_builder = std::make_unique<std::tuple<HtmlStringWriter, std::wstring*>>(
                HtmlStringWriter(), GetReportTextBuilderWithValidityCheck(report));

            if( std::get<1>(*html_writer_and_report_text_builder) == nullptr )
                return 0;

            frequency_printer = std::make_shared<HtmlFrequencyPrinter>(std::get<0>(*html_writer_and_report_text_builder), false);
        }

        // ... or to the default frequency printer
        else
        {
            frequency_printer = m_frequencyDriver->GetDefaultFrequencyPrinter();
        }


        // evaluate the optional printing options and save the frequencies
        m_frequencyDriver->PrintFrequencies(named_frequency.GetFrequencyIndex(), *frequency_printer,
                                            named_frequency.GetName(), EvaluateDynamicFrequencyPrinterOptions(this, frequency, arguments[2]));


        // write the frequencies to the report
        if( html_writer_and_report_text_builder != nullptr )
            std::get<1>(*html_writer_and_report_text_builder)->append(std::get<0>(*html_writer_and_report_text_builder).str());
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 94531, exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exFreq_view(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const NamedFrequency& named_frequency = GetSymbolLogicNamedFrequency(symbol_va_node.symbol_index);

    std::unique_ptr<const ViewerOptions> viewer_options;
    int frequency_parameters_node_index;

    if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_3) )
    {
        viewer_options = EvaluateViewerOptions(symbol_va_node.arguments[0]);
        frequency_parameters_node_index = symbol_va_node.arguments[1];
    }

    else
    {
        frequency_parameters_node_index = symbol_va_node.arguments[0];
    }

    return exFreq_view(named_frequency, viewer_options.get(), frequency_parameters_node_index);
}


double CIntDriver::exFreq_view(const NamedFrequency& named_frequency, const ViewerOptions* viewer_options, const int frequency_parameters_node_index)
{
    const Frequency& frequency = *m_engineData->frequencies[named_frequency.GetFrequencyIndex()];
    bool success = false;

    try
    {
        HtmlStringWriter html_writer;
        HtmlFrequencyPrinter frequency_printer(html_writer, true);

        // evaluate the optional printing options and write the frequencies to a string stream
        m_frequencyDriver->PrintFrequencies(named_frequency.GetFrequencyIndex(), frequency_printer,
                                            named_frequency.GetName(),
                                            EvaluateDynamicFrequencyPrinterOptions(this, frequency, frequency_parameters_node_index));

        Viewer viewer;
        success = viewer.UseEmbeddedViewer()
                        .SetOptions(viewer_options)
                        .ViewHtmlContent(html_writer.str());
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 94531, exception.GetErrorMessage().c_str());
    }

    return success ? 1 : 0;
}


double CIntDriver::exfreqvar(int iExpr)
{
    return m_frequencyDriver->GetSingleFrequencyCounterCount(iExpr);
}


double CIntDriver::exfreqcompute(int iExpr)
{
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(iExpr);
    ASSERT(symbol_compute_node.rhs_symbol_type == SymbolType::None);
    m_frequencyDriver->SetSingleFrequencyCounterCount(symbol_compute_node.lhs_symbol_index, evalexpr(symbol_compute_node.rhs_symbol_index));
    return 0;
}
