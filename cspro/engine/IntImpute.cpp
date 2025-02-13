#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "FrequencyDriver.h"
#include "ImputationDriver.h"
#include "ParadataDriver.h"
#include <zEngineO/Imputation.h>
#include <zEngineO/ValueSet.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/Impute.h>
#include <zToolsO/Hash.h>
#include <ZBRIDGEO/npff.h>
#include <zParadataO/Logger.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseItemHelpers.h>
#include <zCaseO/FixedWidthNumericCaseItem.h>
#include <zCaseO/FixedWidthStringCaseItem.h>
#include <zFreqO/FrequencyPrinter.h>
#include <zFreqO/FrequencyPrinterEntry.h>
#include <zFreqO/FrequencyPrinterOptions.h>
#include <zFreqO/FrequencyTable.h>


// --------------------------------------------------------------------------
// the imputation driver implementation
// --------------------------------------------------------------------------

ImputationDriver::ImputationDriver(CIntDriver& int_driver)
    :   m_pIntDriver(&int_driver),
        m_engineData(m_pIntDriver->m_engineData),
        m_pEngineDriver(m_pIntDriver->m_pEngineDriver),
        m_statCaseIdRecord(nullptr),
        m_statCaseKeyIncrementer(0)
{
    // set up the imputation frequencies
    for( const std::shared_ptr<Imputation>& imputation : m_engineData->imputations )
    {
        if( imputation->GetVariable()->IsNumeric() )
        {
            SetupImputationFrequency(m_numericImputationFrequencies, imputation);
        }

        else
        {
            SetupImputationFrequency(m_stringImputationFrequencies, imputation);
        }
    }

    // warn if no imputation frequencies filename was provided
    if( m_pEngineDriver->GetPifFile()->GetImputeFrequenciesFilename().IsEmpty() )
        issaerror(MessageType::Warning, 8111);


    // setup the stat data file
    if( m_pEngineDriver->GetApplication()->GetHasImputeStatStatements() )
    {
        // warn if no stat connection string was provided
        if( !m_pEngineDriver->m_pPifFile->GetImputeStatConnectionString().IsDefined() )
        {
            issaerror(MessageType::Warning, 8112);
        }

        else
        {
            SetupStatDataFile();
        }
    }
};


ImputationDriver::~ImputationDriver()
{
    WriteFrequencies();

    // close the stat data file
    if( m_statDataRepository != nullptr)
    {
        // write the last stat case (if it exists)
        WriteStatCase();

        try
        {
            m_statDataRepository->Close();
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Error, 8114, exception.GetErrorMessage().c_str());
        }
    }
}


template<typename T>
void ImputationDriver::SetupImputationFrequency(std::vector<ImputationFrequency<T>>& imputation_frequencies, std::shared_ptr<Imputation> imputation)
{
    // imputation frequencies will be combined if they:
    // - do not have specific set
    // - use the same item
    // - use the same value set
    std::optional<size_t> imputation_frequency_index;

    if( !imputation->GetSpecific() )
    {
        const auto& imputation_search = std::find_if(m_engineData->imputations.cbegin(), m_engineData->imputations.cend(),
            [&](const auto& compare_imputation)
            {
                return ( !compare_imputation->GetSpecific() &&
                         imputation->GetVariable() == compare_imputation->GetVariable() &&
                         imputation->GetValueSet() == compare_imputation->GetValueSet() );
            });

        if( *imputation_search != imputation )
            imputation_frequency_index = (*imputation_search)->GetImputationFrequencyIndex();
    }

    // create a new imputation frequency if necessary
    if( !imputation_frequency_index.has_value() )
    {
        imputation_frequency_index = imputation_frequencies.size();

        imputation_frequencies.emplace_back(ImputationFrequency<T>
            {
                imputation,
                FrequencyCounter<T, size_t>::Create(imputation->GetVariable()->GetDictItem())
            });

        m_imputationFrequenciesPrintingOrder.emplace_back(imputation->GetVariable()->IsNumeric());
    }

    else
    {
        // update the title if one is set
        if( imputation->GetTitle().has_value() )
            imputation_frequencies[*imputation_frequency_index].imputation->SetTitle(*imputation->GetTitle());
    }

    imputation->SetImputationFrequencyIndex(*imputation_frequency_index);
}


namespace
{
    template<typename T>
    std::unique_ptr<FrequencyTable> CreateImputationFrequencyTable(FrequencyPrinterOptions& frequency_printer_options,
                                                                   const ImputationFrequency<T>& imputation_frequency)
    {
        const CDictItem* dict_item = imputation_frequency.imputation->GetVariable()->GetDictItem();
        const DictValueSet* dict_value_set;
        bool distinct;

        // when specifying a value set, imputed values will be collapsed into the values from the value set
        if( imputation_frequency.imputation->GetValueSet() != nullptr )
        {
            dict_value_set = &imputation_frequency.imputation->GetValueSet()->GetDictValueSet();
            distinct = false;
        }

        // otherwise, each imputed value will be shown
        else
        {
            dict_value_set = dict_item->GetFirstValueSetOrNull();
            distinct = true;
        }

        FrequencyPrinterEntry<T, size_t> frequency_print_entry(imputation_frequency.frequency_counter,
            *dict_item, nullptr, std::nullopt, std::nullopt);

        auto frequency_table = frequency_print_entry.CreateFrequencyTable(std::wstring(), frequency_printer_options, dict_value_set, distinct);

        // override the table title
        if( imputation_frequency.imputation->GetTitle().has_value() )
        {
            frequency_table->titles = { *imputation_frequency.imputation->GetTitle() };
        }

        else
        {
            // if no table title is specified, use the default frequency title (but modify it
            // to prefix it with imputed)
            ASSERT(!frequency_table->titles.empty() && frequency_table->titles.front().find(_T("Item")) == 0);
            frequency_table->titles.resize(1);
            frequency_table->titles.front().insert(0, _T("Imputed "));
        }

        frequency_table->special_formatting = FrequencyTable::SpecialFormatting::CenterTitles;

        return frequency_table;
    }
}


void ImputationDriver::WriteFrequencies()
{
    if( m_pEngineDriver->m_pPifFile->GetImputeFrequenciesFilename().IsEmpty() )
        return;

    try
    {
        // open the frequency file
        std::unique_ptr<FrequencyPrinter> frequency_printer = FrequencyDriver::CreateFrequencyPrinter(
            CS2WS(m_pEngineDriver->m_pPifFile->GetImputeFrequenciesFilename()), *m_pEngineDriver->m_pPifFile);

        FrequencyPrinterOptions frequency_printer_options;
        frequency_printer_options.SetSortType(FrequencyPrinterOptions::SortType::ByCode);
        frequency_printer_options.SetHeadings({ _T("IMPUTE FREQUENCIES") });

        // print the frequencies
        frequency_printer->StartFrequencyGroup();

        auto numeric_imputation_frequency_itr = m_numericImputationFrequencies.cbegin();
        auto string_imputation_frequency_itr = m_stringImputationFrequencies.cbegin();

        for( bool print_numeric_imputation : m_imputationFrequenciesPrintingOrder )
        {
            std::unique_ptr<FrequencyTable> frequency_table;

            if( print_numeric_imputation )
            {
                frequency_table = CreateImputationFrequencyTable(frequency_printer_options, *(numeric_imputation_frequency_itr++));
            }

            else
            {
                frequency_table = CreateImputationFrequencyTable(frequency_printer_options, *(string_imputation_frequency_itr++));
            }

            frequency_printer->Print(*frequency_table);
        }
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 8115, exception.GetErrorMessage().c_str());
    }
}


namespace
{
    enum class ImputeNameType { Record, Initial, Imputed, Key, Stat, LineNumber, CompilationUnit };

    struct ImputationsAndStatVariables
    {
        std::vector<std::shared_ptr<Imputation>> imputations;
        std::vector<const VART*> stat_variables;
        size_t max_occurrences;
        std::vector<std::tuple<ImputeNameType, const CDictItem*>> impute_item_types;
    };


    class StatDictionaryWorker
    {
    private:
        constexpr static wstring_view NamePrefix     = _T("IMPUTE_");
        constexpr static size_t KeyIncrementerLength = 2;

    public:
        StatDictionaryWorker(std::vector<ImputationsAndStatVariables>& imputations_and_stat_variables, const CDataDict* source_dictionary)
            :   m_imputationsAndStatVariables(imputations_and_stat_variables),
                m_sourceDictionary(source_dictionary),
                m_sourceIdItemsByLevel(m_sourceDictionary->GetIdItemsByLevel())
        {
        }


        std::shared_ptr<CDataDict> CreateDictionary()
        {
            m_statDictionary = std::make_shared<CDataDict>();

            // setup the dictionary and the level
            m_statDictionary->SetName(NamePrefix + m_sourceDictionary->GetName());
            m_statDictionary->SetLabel(CString(_T("(Impute) ")) + m_sourceDictionary->GetLabel());
            m_statDictionary->SetPosRelative(true);
            m_statDictionary->SetRecTypeStart(1);
            m_statDictionary->CopyDictionarySettings(*m_sourceDictionary);

            const DictLevel& source_dict_level = m_sourceDictionary->GetLevel(0);

            DictLevel dict_level;
            dict_level.SetName(NamePrefix + source_dict_level.GetName());
            dict_level.SetLabel(source_dict_level.GetLabel());


            // add the record type and first level ID items
            size_t record_length = GetLengthToStoreValue(m_imputationsAndStatVariables.size());
            CString record_length_formatter = FormatText(_T("%%0%dd"), static_cast<int>(record_length));
            m_statDictionary->SetRecTypeLen(record_length);

            CDictRecord* destination_id_record = dict_level.GetIdItemsRec();
            destination_id_record->SetRecLen(m_statDictionary->GetRecTypeLen());

            for( const CDictItem* id_item : m_sourceIdItemsByLevel.front() )
                CopyItem(*id_item, *destination_id_record, NamePrefix + id_item->GetName());

            // add the key incrementer ID item
            CDictItem key_incrementer_item;
            key_incrementer_item.SetName(_T("KEY_INCREMENTER"));
            key_incrementer_item.SetLabel(_T("Key Incrementer"));
            key_incrementer_item.SetContentType(ContentType::Numeric);
            key_incrementer_item.SetLen(KeyIncrementerLength);
            key_incrementer_item.SetZeroFill(m_statDictionary->IsZeroFill());
            CopyItem(key_incrementer_item, *destination_id_record, NamePrefix + key_incrementer_item.GetName());


            // calculate the lengths of the line number and compilation units and
            // create items for those two values (that will be put on each record)
            size_t max_line_number = 0;
            size_t compilation_unit_length = 0;

            for( const ImputationsAndStatVariables& imputations_and_stat_variables : m_imputationsAndStatVariables )
            {
                for( const std::shared_ptr<Imputation>& imputation : imputations_and_stat_variables.imputations )
                {
                    max_line_number = std::max(max_line_number, imputation->GetLineNumber());
                    compilation_unit_length = std::max(compilation_unit_length, imputation->GetCompilationUnit().length());
                }
            }

            m_lineNumberItem.SetName(_T("LINE_NUMBER"));
            m_lineNumberItem.SetLabel(_T("Line Number"));
            m_lineNumberItem.SetContentType(ContentType::Numeric);
            m_lineNumberItem.SetLen(GetLengthToStoreValue(max_line_number));
            m_lineNumberItem.SetZeroFill(m_statDictionary->IsZeroFill());

            if( compilation_unit_length != 0 )
            {
                m_compilationUnitItem = std::make_unique<CDictItem>();
                m_compilationUnitItem->SetName(_T("COMPILATION_UNIT"));
                m_compilationUnitItem->SetLabel(_T("Compilation Unit"));
                m_compilationUnitItem->SetContentType(ContentType::Alpha);
                m_compilationUnitItem->SetLen(std::min(compilation_unit_length, static_cast<size_t>(MAX_ALPHA_ITEM_LEN)));
            }


            // create the imputation records...the records will be created in order sorted by variable name
            std::sort(m_imputationsAndStatVariables.begin(), m_imputationsAndStatVariables.end(),
                [](const auto& iasv1, const auto& iasv2)
                {
                    return ( SO::CompareNoCase(iasv1.imputations.front()->GetVariable()->GetName(), iasv2.imputations.front()->GetVariable()->GetName()) < 0 );
                });

            int record_counter = 0;

            for( ImputationsAndStatVariables& imputations_and_stat_variables : m_imputationsAndStatVariables )
            {
                CDictRecord impute_record;
                impute_record.SetRecLen(destination_id_record->GetRecLen());
                impute_record.SetRecTypeVal(FormatText(record_length_formatter, ++record_counter));

                FillRecord(imputations_and_stat_variables, impute_record);

                dict_level.AddRecord(&impute_record);
            }


            // finalize the dictionary
            m_statDictionary->AddLevel(std::move(dict_level));
            m_statDictionary->UpdatePointers();

            return m_statDictionary;
        }


        std::shared_ptr<CaseAccess> CreateCaseAccess()
        {
            m_statCaseAccess = std::make_shared<CaseAccess>(*m_statDictionary);

            m_statCaseAccess->SetUseAllDictionaryItems();
            m_statCaseAccess->Initialize();

            return m_statCaseAccess;
        }


        std::vector<StatRecord> CreateStatRecords(CaseLevel& root_case_level)
        {
            std::vector<StatRecord> stat_records;
            size_t case_record_index = 0;

            for( ImputationsAndStatVariables& imputations_and_stat_variables : m_imputationsAndStatVariables )
            {
                for( const std::shared_ptr<Imputation>& imputation : imputations_and_stat_variables.imputations )
                {
                    imputation->SetStatRecordIndex(stat_records.size());

                    StatRecord& stat_record = stat_records.emplace_back();
                    stat_record.case_record = &root_case_level.GetCaseRecord(case_record_index);
                    stat_record.compilation_unit_case_item = nullptr;

                    std::vector<std::tuple<const CDictItem*, const CaseItem*>> stat_case_items;

                    size_t item_index = 0;

                    for( const auto& [type, source_item] : imputations_and_stat_variables.impute_item_types )
                    {
                        const CaseItem*& case_item =
                            ( type == ImputeNameType::Initial )           ?   stat_record.initial_case_item :
                            ( type == ImputeNameType::Imputed )           ?   stat_record.imputed_case_item :
                            ( type == ImputeNameType::Key )               ?   stat_record.key_case_items.emplace_back() :
                            ( type == ImputeNameType::Stat )              ?   std::get<1>(stat_case_items.emplace_back(source_item, nullptr)) :
                            ( type == ImputeNameType::LineNumber )        ?   stat_record.line_number_case_item :
                          /*( type == ImputeNameType::CompilationUnit )   ? */stat_record.compilation_unit_case_item;

                        case_item = &stat_record.case_record->GetCaseItem(item_index);

                        ++item_index;
                    }

                    ASSERT(item_index == stat_record.case_record->GetNumberCaseItems());

                    // not all imputations for the same variable will use the same stat variables, so we need to filter them in the correct order
                    for( const VART* stat_variable : imputation->GetStatVariables() )
                    {
                        auto stat_case_item_lookup = std::find_if(stat_case_items.cbegin(), stat_case_items.cend(),
                            [&](const auto& stat_case_item) { return ( stat_variable->GetDictItem() == std::get<0>(stat_case_item) ); });

                        ASSERT(stat_case_item_lookup != stat_case_items.cend());

                        stat_record.stat_case_items.emplace_back(std::get<1>(*stat_case_item_lookup));
                        stat_case_items.erase(stat_case_item_lookup);
                    }

                    ASSERT(stat_record.stat_case_items.size() == imputation->GetStatVariables().size());
                }

                ++case_record_index;
            }

            return stat_records;
        }

    private:
        static size_t GetLengthToStoreValue(size_t value)
        {
            return static_cast<size_t>(std::floor(std::log10(value))) + 1;
        }


        CString GetImputeName(const CDictItem* item, ImputeNameType type, const CDictItem* additional_item = nullptr)
        {
            CString name = NamePrefix + item->GetName() + _T("_") +
                (
                    ( type == ImputeNameType::Record )            ?   _T("REC") :
                    ( type == ImputeNameType::Initial )           ?   _T("INITIAL") :
                    ( type == ImputeNameType::Imputed )           ?   _T("IMPUTED") :
                    ( type == ImputeNameType::Key )               ?   _T("KEY") :
                    ( type == ImputeNameType::Stat )              ?   _T("STAT") :
                    ( type == ImputeNameType::LineNumber )        ?   _T("LINE_NUMBER") :
                    /*( type == ImputeNameType::CompilationUnit ) ? */_T("COMPILATION_UNIT")
                );

            return ( additional_item != nullptr ) ? ( name + _T("_") + additional_item->GetName() ) : name;
        }


        void CopyItem(const CDictItem& source_dict_item, CDictRecord& dest_dict_record,
                      const CString& name, std::optional<CString> label = std::nullopt)
        {
            CDictItem dest_dict_item;
            dest_dict_item.SetName(name);
            dest_dict_item.SetLabel(label.has_value() ? *label : source_dict_item.GetLabel());
            dest_dict_item.SetNote(source_dict_item.GetNote());
            dest_dict_item.SetContentType(source_dict_item.GetContentType());
            dest_dict_item.SetStart(dest_dict_record.GetRecLen() + 1);
            dest_dict_item.SetLen(source_dict_item.GetLen());
            dest_dict_item.SetDecimal(source_dict_item.GetDecimal());
            dest_dict_item.SetDecChar(source_dict_item.GetDecChar());
            dest_dict_item.SetZeroFill(source_dict_item.GetZeroFill());

            dest_dict_record.SetRecLen(dest_dict_record.GetRecLen() + source_dict_item.GetLen());

            // copy the item's value sets, using linked value sets to minimize the number of value sets
            int i = 0;

            for( const DictValueSet& source_dict_value_set : source_dict_item.GetValueSets() )
            {
                DictValueSet dest_dict_value_set = source_dict_value_set;
                dest_dict_value_set.SetName(FormatText(_T("%s_VS%d"), dest_dict_item.GetName().GetString(), ++i));

                // if the source value set was not a linked value set, create a fake link because
                // then the initial and imputed, and potentially stat values, can be linked
                if( !dest_dict_value_set.IsLinkedValueSet() )
                    dest_dict_value_set.LinkValueSetByCode(Hash::Hash(source_dict_value_set.GetName()));

                dest_dict_item.AddValueSet(std::move(dest_dict_value_set));
            }

            dest_dict_record.AddItem(&dest_dict_item);
        }


        void FillRecord(ImputationsAndStatVariables& imputations_and_stat_variables, CDictRecord& impute_record)
        {
            const CDictItem* impute_item = imputations_and_stat_variables.imputations.front()->GetVariable()->GetDictItem();

            impute_record.SetName(GetImputeName(impute_item, ImputeNameType::Record));
            impute_record.SetLabel(impute_item->GetLabel());
            impute_record.SetRequired(false);
            impute_record.SetMaxRecs(imputations_and_stat_variables.max_occurrences);

            // because a stat variable can occur more than once, we may need to add a suffix to make the name unique
            std::set<const VART*> seen_stat_variables;
            std::map<const CDictItem*, int> stat_variables_next_occurrence_map;

            for( const VART* stat_variable : imputations_and_stat_variables.stat_variables )
            {
                if( seen_stat_variables.find(stat_variable) == seen_stat_variables.cend() )
                {
                    seen_stat_variables.insert(stat_variable);
                }

                else
                {
                    stat_variables_next_occurrence_map[stat_variable->GetDictItem()] = 1;
                }
            }

            IterateOverRecordItems(imputations_and_stat_variables,
                [&](const CDictItem* source_item, ImputeNameType type, CString new_item_name)
                {
                    CString label_prefix = ( type == ImputeNameType::Initial )  ? _T("(Initial) ") :
                                           ( type == ImputeNameType::Imputed )  ? _T("(Imputed) ") :
                                           ( type == ImputeNameType::Key )      ? _T("(Key) ") :
                                           ( type == ImputeNameType::Stat )     ? _T("(Stat) ") :
                                                                                  _T("");

                    if( type == ImputeNameType::Stat )
                    {
                        auto occurrence_lookup = stat_variables_next_occurrence_map.find(source_item);

                        if( occurrence_lookup != stat_variables_next_occurrence_map.end() )
                        {
                            new_item_name.AppendFormat(_T("_%d"), occurrence_lookup->second);
                            ++occurrence_lookup->second;
                        }
                    }

                    CopyItem(*source_item, impute_record, new_item_name, label_prefix + source_item->GetLabel());

                    imputations_and_stat_variables.impute_item_types.emplace_back(type, source_item);
                });
        }


        template<typename CallbackFunction>
        void IterateOverRecordItems(const ImputationsAndStatVariables& imputations_and_stat_variables, CallbackFunction callback_function)
        {
            const CDictItem* impute_item = imputations_and_stat_variables.imputations.front()->GetVariable()->GetDictItem();

            // add the initial and imputed values
            callback_function(impute_item, ImputeNameType::Initial, GetImputeName(impute_item, ImputeNameType::Initial));
            callback_function(impute_item, ImputeNameType::Imputed, GetImputeName(impute_item, ImputeNameType::Imputed));

            // if this is not on the main level, add any ID items
            for( size_t level_number = 1; level_number <= impute_item->GetLevel()->GetLevelNumber(); ++level_number )
            {
                for( const CDictItem* id_item : m_sourceIdItemsByLevel[level_number] )
                    callback_function(id_item, ImputeNameType::Key, GetImputeName(impute_item, ImputeNameType::Key, id_item));
            }

            // add any stat variables
            for( const VART* stat_variable : imputations_and_stat_variables.stat_variables )
            {
                callback_function(stat_variable->GetDictItem(), ImputeNameType::Stat,
                    GetImputeName(impute_item, ImputeNameType::Stat, stat_variable->GetDictItem()));
            }

            // add the line number and compilation unit
            callback_function(&m_lineNumberItem, ImputeNameType::LineNumber,
                GetImputeName(impute_item, ImputeNameType::LineNumber));

            if( m_compilationUnitItem != nullptr )
                callback_function(m_compilationUnitItem.get(), ImputeNameType::CompilationUnit,
                    GetImputeName(impute_item, ImputeNameType::CompilationUnit));
        }


    private:
        std::vector<ImputationsAndStatVariables>& m_imputationsAndStatVariables;

        const CDataDict* m_sourceDictionary;
        const std::vector<std::vector<const CDictItem*>> m_sourceIdItemsByLevel;

        CDictItem m_lineNumberItem;
        std::unique_ptr<CDictItem> m_compilationUnitItem;

        std::shared_ptr<CDataDict> m_statDictionary;
        std::shared_ptr<CaseAccess> m_statCaseAccess;
    };
}


void ImputationDriver::SetupStatDataFile()
{
    // there is no reason to setup the stat data file is this information will not be written out
    if( !m_pEngineDriver->m_pPifFile->GetImputeStatConnectionString().IsFilenamePresent() )
        return;

    // first determine the imputations that are being used for stat and
    // any additional variables that will be part of each record
    std::vector<ImputationsAndStatVariables> imputations_and_stat_variables;

    for( const std::shared_ptr<Imputation>& imputation : m_engineData->imputations )
    {
        if( imputation->GetUsingStat() )
        {
            // see if there is already an entry for this variable
            auto imputations_and_stat_variables_lookup = std::find_if(imputations_and_stat_variables.begin(),
                                                                      imputations_and_stat_variables.end(),
                [&](const auto& iasv)
                {
                    return ( iasv.imputations.front()->GetVariable() == imputation->GetVariable() );
                });

            if( imputations_and_stat_variables_lookup != imputations_and_stat_variables.end() )
            {
                imputations_and_stat_variables_lookup->imputations.emplace_back(imputation);

                // add any new stat variables
                std::map<const VART*, size_t> last_found_index_map;

                for( const VART* stat_variable : imputation->GetStatVariables() )
                {
                    const auto& last_found_index_map_lookup = last_found_index_map.find(stat_variable);
                    size_t index = ( last_found_index_map_lookup != last_found_index_map.cend() ) ? ( last_found_index_map_lookup->second + 1 ) : 0;

                    for( ; index < imputations_and_stat_variables_lookup->stat_variables.size(); ++index )
                    {
                        if( stat_variable == imputations_and_stat_variables_lookup->stat_variables[index] )
                        {
                            last_found_index_map[stat_variable] = index;
                            break;
                        }
                    }

                    if( index == imputations_and_stat_variables_lookup->stat_variables.size() )
                    {
                        last_found_index_map[stat_variable] = imputations_and_stat_variables_lookup->stat_variables.size();
                        imputations_and_stat_variables_lookup->stat_variables.emplace_back(stat_variable);
                    }
                }
            }

            // add a new entry
            else
            {
                const CDictItem* dict_item = imputation->GetVariable()->GetDictItem();

                imputations_and_stat_variables.emplace_back(ImputationsAndStatVariables
                    {
                        { imputation },
                        imputation->GetStatVariables(),
                        std::min((unsigned int)MAX_MAX_RECS, dict_item->GetRecord()->GetMaxRecs() * dict_item->GetOccurs())
                    });
            }
        }
    }

    ASSERT(!imputations_and_stat_variables.empty());


    // create the dictionary describing the imputations
    StatDictionaryWorker stat_dictionary_worker(imputations_and_stat_variables, m_pIntDriver->m_engineData->dictionaries_pre80.front()->GetDataDict());
    m_statDictionary = stat_dictionary_worker.CreateDictionary();

    // the dictionary will be saved using the name of the data file but with a different extension
    const ConnectionString& stat_connection_string = m_pEngineDriver->m_pPifFile->GetImputeStatConnectionString();
    std::wstring data_filename = stat_connection_string.GetFilename();

    std::wstring dictionary_filename = PortableFunctions::PathAppendToPath(PortableFunctions::PathGetDirectory(data_filename),
        PortableFunctions::PathGetFilenameWithoutExtension(data_filename) + FileExtensions::WithDot::Dictionary);

    try
    {
        m_statDictionary->Save(dictionary_filename);
    }

    catch( const CSProException& )
    {
        issaerror(MessageType::Error, 8113);
        return;
    }


    // setup the case access and case item links
    m_statCaseAccess = stat_dictionary_worker.CreateCaseAccess();
    m_statCase = m_statCaseAccess->CreateCase();

    m_statCaseIdRecord = &m_statCase->GetRootCaseLevel().GetIdCaseRecord();
    m_statRecords = stat_dictionary_worker.CreateStatRecords(m_statCase->GetRootCaseLevel());


    // open the data file
    try
    {
        m_statDataRepository = DataRepository::CreateAndOpen(m_statCaseAccess, stat_connection_string,
            DataRepositoryAccess::BatchOutput, DataRepositoryOpenFlag::CreateNew);
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Error, 8114, exception.GetErrorMessage().c_str());
    }
}


void ImputationDriver::WriteStatCase()
{
    try
    {
        if( !m_statCaseUuid.empty() )
            m_statDataRepository->WriteCase(*m_statCase);
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Error, 8114, exception.GetErrorMessage().c_str());
    }

}


template<typename T>
void ImputationDriver::ProcessStat(const Imputation& imputation, const T& initial_value, const T& imputed_value,
    const Nodes::List& stat_variable_compilations)
{
    if( m_statDataRepository == nullptr )
        return;

    const StatRecord& stat_record = m_statRecords[imputation.GetStatRecordIndex()];

    // see if a new case has to be created, which will occur if this case is different from the previous case,
    // or if we have reached the maximum number of occurrences for this record
    DICX* pDicX = DIX(0);
    Case& input_case = pDicX->GetCase();
    const CString& input_case_uuid = input_case.GetOrCreateUuid();
    bool case_is_new = !SO::EqualsNoCase(m_statCaseUuid, input_case_uuid);

    if( case_is_new || stat_record.case_record->GetNumberOccurrences() == stat_record.case_record->GetCaseRecordMetadata().GetDictionaryRecord().GetMaxRecs() )
    {
        WriteStatCase();

        m_statCase->Reset();

        if( case_is_new )
        {
            m_statCaseUuid = input_case_uuid;
            m_statCaseKeyIncrementer = 1;
        }

        else
        {
            ++m_statCaseKeyIncrementer;
        }

        // copy over the IDs
        // CR_TODO ... eventually these values may not reflect the case key when this case was loaded, so
        // we will need to figure out a way to get the original key
        const CaseRecord& input_id_case_record = input_case.GetRootCaseLevel().GetIdCaseRecord();
        ASSERT(m_statCaseIdRecord->GetNumberCaseItems() == ( input_id_case_record.GetNumberCaseItems() + 1 ));

        const CaseItemIndex input_id_index = input_id_case_record.GetCaseItemIndex();
        CaseItemIndex stat_id_index = m_statCaseIdRecord->GetCaseItemIndex();

        auto stat_id_case_item_itr = m_statCaseIdRecord->GetCaseItems().cbegin();

        for( const CaseItem* input_id_case_item : input_id_case_record.GetCaseItems() )
        {
            CaseItemHelpers::CopyValue(*input_id_case_item, input_id_index, *(*stat_id_case_item_itr), stat_id_index);
            ++stat_id_case_item_itr;
        }

        // set the case key incrementer
        CaseItemHelpers::SetValue(*(*stat_id_case_item_itr), stat_id_index, m_statCaseKeyIncrementer);
    }


    // set the base stat record values
    size_t stat_record_occurence = stat_record.case_record->GetNumberOccurrences();
    stat_record.case_record->SetNumberOccurrences(stat_record_occurence + 1);

    CaseItemIndex stat_record_index = stat_record.case_record->GetCaseItemIndex(stat_record_occurence);

    CaseItemHelpers::SetValue(*stat_record.initial_case_item, stat_record_index, initial_value);
    CaseItemHelpers::SetValue(*stat_record.imputed_case_item, stat_record_index, imputed_value);

    CaseItemHelpers::SetValue(*stat_record.line_number_case_item, stat_record_index, imputation.GetLineNumber());

    if( stat_record.compilation_unit_case_item != nullptr )
        CaseItemHelpers::SetValue(*stat_record.compilation_unit_case_item, stat_record_index, imputation.GetCompilationUnit());


    // set the multiple level key values
    // CR_TODO see note above about modified key values
    if( !stat_record.key_case_items.empty() )
    {
        // CR_TODO for now this will get the values from the key, but eventually it should access the CaseLevel's ID case record
        const TCHAR* key_itr = pDicX->current_key + input_case.GetKey().GetLength();

        for( const CaseItem* key_case_item : stat_record.key_case_items )
        {
            ASSERT(key_case_item->IsTypeFixed());

            if( key_case_item->IsTypeNumeric() )
            {
                assert_cast<const FixedWidthNumericCaseItem*>(key_case_item)->SetValueFromTextInput(stat_record_index, key_itr);
            }

            else
            {
                assert_cast<const FixedWidthStringCaseItem*>(key_case_item)->SetFixedWidthValue(stat_record_index, key_itr);
            }

            key_itr += key_case_item->GetDictionaryItem().GetLen();
        }
    }


    // set the stat values
    ASSERT(stat_record.stat_case_items.size() == static_cast<size_t>(stat_variable_compilations.number_elements));
    const int* expression_itr = stat_variable_compilations.elements;

    for( const CaseItem* stat_case_item : stat_record.stat_case_items )
    {
        if( stat_case_item->IsTypeNumeric() )
        {
            CaseItemHelpers::SetValue(*stat_case_item, stat_record_index, m_pIntDriver->EvaluateVARTValue<double>(*expression_itr));
        }

        else
        {
            CaseItemHelpers::SetValue(*stat_case_item, stat_record_index, m_pIntDriver->EvaluateVARTValue<std::wstring>(*expression_itr));
        }

        ++expression_itr;
    }
}



// --------------------------------------------------------------------------
// the impute function
// --------------------------------------------------------------------------

template<typename T, typename TIN, typename TIO>
double CIntDriver::eximpute_worker(const TIN& impute_node, TIO imputation)
{
    ASSERT(m_imputationDriver != nullptr);

    std::shared_ptr<Paradata::FieldInfo> paradata_field_info;

    T initial_value;
    T imputed_value;

    ModifyVARTValue<T>(impute_node.variable_compilation,
        [&](T& value)
        {
            // store the current value
            initial_value = value;

            // assign the new value
            value = EvaluateExpression<T>(impute_node.value_expression);
            imputed_value = value;

        }, Paradata::Logger::IsOpen() ? &paradata_field_info : nullptr);


    // log the paradata event
    if( paradata_field_info != nullptr )
    {
        // only numeric imputations are recorded
        if constexpr(std::is_same_v<T, double>)
            m_pParadataDriver->RegisterAndLogEvent(std::make_shared<Paradata::ImputeEvent>(paradata_field_info, initial_value, imputed_value));
    }


    // update the imputation frequency
    auto& imputation_frequency = m_imputationDriver->GetImputationFrequency<T>(imputation.GetImputationFrequencyIndex());
    imputation_frequency.frequency_counter->Add(imputed_value, 1);

    // update a (non-string literal) title
    if( impute_node.title_expression != -1 )
        imputation_frequency.imputation->SetTitle(EvalAlphaExpr(impute_node.title_expression));


    // process the stat variables
    if( imputation.GetUsingStat() )
    {
        const Nodes::List& stat_variable_compilations = GetOptionalListNode(impute_node.stat_variable_compilation_list_node);
        m_imputationDriver->ProcessStat(imputation, initial_value, imputed_value, stat_variable_compilations);
    }


    // return the imputed value (or 1 if imputing an alphanumeric variable)
    if constexpr(std::is_same_v<T, double>)
    {
        return imputed_value;
    }

    else
    {
        return 1;
    }
}


double CIntDriver::eximpute(int iExpr)
{
    if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1) )
    {
        // for old .pen files, simply execute the compute node assignment (which was located after the IMPUTE_NODE)
        evalexpr(iExpr + 11);
        return 0;
    }

    const auto& impute_node = GetNode<Nodes::Impute>(iExpr);
    Imputation& imputation = *m_engineData->imputations[impute_node.imputation_index];

    return imputation.GetVariable()->IsNumeric() ? eximpute_worker<double>(impute_node, imputation) :
                                                   eximpute_worker<std::wstring>(impute_node, imputation);
}
