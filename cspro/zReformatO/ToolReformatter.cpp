#include "stdafx.h"
#include "ToolReformatter.h"
#include "Reformatter.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zToolsO/NumberToString.h>
#include <zToolsO/TextFile.h>
#include <zUtilO/BasicLogger.h>
#include <zUtilF/ProcessSummaryDlg.h>
#include <zAppO/PFF.h>
#include <zCaseO/StdioCaseConstructionReporter.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DictionarySource.h>


namespace
{
    constexpr size_t ProgressBarCaseUpdateFrequency = 100;
}


void ToolReformatter::GetDictionaryDifferences(BasicLogger& differences_log, const Reformatter& reformatter,
                                               const bool show_names, const bool show_only_destructive_changes,
                                               size_t* const out_display_name_length/* = nullptr*/)
{
    const std::vector<DictionaryDifference> differences = GetSortedDifferences(reformatter);
    const DictionaryDifference* previous_difference = nullptr;

    // space out the report based on the maximum name/label length
    constexpr size_t MinDisplayNameLength = 8;
    constexpr size_t MaxDisplayNameLength = 48;
    size_t display_name_length = MinDisplayNameLength;

    for( const DictionaryDifference& difference : differences )
        display_name_length = std::max(display_name_length, SO::WideLength(difference.GetDisplayName(show_names)));

    display_name_length = std::min(display_name_length, MaxDisplayNameLength);

    if( out_display_name_length != nullptr )
        *out_display_name_length = display_name_length;

    const std::string difference_formatter = FormatText("    %%-%ds  |  %%s", static_cast<int>(display_name_length));

    // process the differences
    for( const DictionaryDifference& difference : differences )
    {
        if( show_only_destructive_changes && !difference.IsDestructive() )
            continue;

        auto add_header = [&](std::string header)
        {
            if( previous_difference != nullptr )
                differences_log.AppendLine();

            differences_log.AppendLine(std::move(header));
        };

        std::string difference_text;

        // level differences
        if( difference.IsLevelDifference() )
        {
            if( previous_difference == nullptr || !previous_difference->IsLevelDifference() )
                add_header("Changes in levels:");

            difference_text = ( difference.type == DictionaryDifference::Type::LevelRemoved ) ? "Removed" :
                              ( difference.type == DictionaryDifference::Type::LevelAdded )   ? "Added" :
                                                                                                ReturnProgrammingError(std::string());
        }


        // record differences
        else if( difference.IsRecordDifference() )
        {
            if( previous_difference == nullptr || !previous_difference->IsRecordDifference() )
                add_header("Changes in records:");

            const CDictRecord* initial_dict_record = assert_nullable_cast<const CDictRecord*>(difference.initial_dict_element);
            const CDictRecord* final_dict_record = assert_nullable_cast<const CDictRecord*>(difference.final_dict_element);

            if( difference.type == DictionaryDifference::Type::RecordRemoved )
            {
                difference_text = "Removed";
            }

            else if( difference.type == DictionaryDifference::Type::RequiredRecordAdded )
            {
                difference_text = "Added (required)";
            }

            else if( difference.type == DictionaryDifference::Type::NonRequiredRecordAdded )
            {
                difference_text = "Added (not required)";
            }

            else if( difference.type == DictionaryDifference::Type::RecordTypeChanged )
            {
                difference_text = FormatText("Record type changed ('%s' -> '%s')",
                                             initial_dict_record->GetRecTypeVal().c_str(),
                                             final_dict_record->GetRecTypeVal().c_str());
            }

            else if( difference.type == DictionaryDifference::Type::RecordOccurrencesDecreased ||
                     difference.type == DictionaryDifference::Type::RecordOccurrencesIncreased )
            {
                difference_text = FormatText("Occurrences changed (%d -> %d)",
                                             static_cast<int>(initial_dict_record->GetMaxRecs()),
                                             static_cast<int>(final_dict_record->GetMaxRecs()));
            }

            else if( difference.type == DictionaryDifference::Type::RequiredRecordOccurrenceAdded )
            {
                difference_text = "Occurrence now required";
            }

            else
            {
                ASSERT(false);
            }
        }


        // item differences
        else
        {
            ASSERT(difference.IsItemDifference());

            if( previous_difference == nullptr || !previous_difference->IsItemDifference() ||
                !SO::EqualsNoCase(assert_cast<const CDictItem*>(previous_difference->GetDefinedDictElement())->GetRecord()->GetName(),
                                  assert_cast<const CDictItem*>(difference.GetDefinedDictElement())->GetRecord()->GetName()) )
            {
                add_header(FormatText("Changes in items on record %s:",
                                       DictionaryDifference::GetDisplayName(assert_cast<const CDictItem*>(difference.GetDefinedDictElement())->GetRecord(), show_names).c_str()));
            }


            const CDictItem* initial_item = assert_nullable_cast<const CDictItem*>(difference.initial_dict_element);
            const CDictItem* final_item = assert_nullable_cast<const CDictItem*>(difference.final_dict_element);

            if( difference.type == DictionaryDifference::Type::ItemRemoved )
            {
                difference_text = "Removed";
            }

            else if( difference.type == DictionaryDifference::Type::ItemAdded )
            {
                difference_text = "Added";
            }

            else if( difference.type == DictionaryDifference::Type::ItemLengthChanged )
            {
                difference_text = FormatText("Length changed (%d -> %d)",
                                             static_cast<int>(initial_item->GetLen()),
                                             static_cast<int>(final_item->GetLen()));
            }

            else if( difference.type == DictionaryDifference::Type::ItemValueTruncated )
            {
                if( initial_item->GetContentType() == ContentType::Numeric && final_item->GetContentType() == ContentType::Numeric )
                {
                    difference_text = FormatText("Value truncated (%d.%d -> %d.%d)",
                                                 static_cast<int>(initial_item->GetIntegerLen()), static_cast<int>(initial_item->GetDecimal()),
                                                 static_cast<int>(final_item->GetIntegerLen()), static_cast<int>(final_item->GetDecimal()));
                }

                else
                {
                    difference_text = FormatText("Value truncated (%d -> %d)",
                                                 static_cast<int>(initial_item->GetLen()),
                                                 static_cast<int>(final_item->GetLen()));
                }
            }

            else if( difference.type == DictionaryDifference::Type::ItemContentTypeChangedValidConversionUnnecessary ||
                     difference.type == DictionaryDifference::Type::ItemContentTypeChangedValidConversionNeeded ||
                     difference.type == DictionaryDifference::Type::ItemContentTypeChangedInvalidSometimes ||
                     difference.type == DictionaryDifference::Type::ItemContentTypeChangedInvalidAlways )
            {
                difference_text = FormatText("Data type changed (%s -> %s)",
                                             ToString(initial_item->GetContentType()),
                                             ToString(final_item->GetContentType()));
            }

            else if( difference.type == DictionaryDifference::Type::ItemFormattingChanged )
            {
                difference_text = "Formatting changed";
            }

            else if( difference.type == DictionaryDifference::Type::ItemMovedOnRecord )
            {
                difference_text = FormatText("Start position changed (%d -> %d)",
                                             static_cast<int>(initial_item->GetStart()),
                                             static_cast<int>(final_item->GetStart()));
            }

            else if( difference.type == DictionaryDifference::Type::ItemMovedToDifferentRecord )
            {
                difference_text = FormatText("Moved to record '%s'",
                                             DictionaryDifference::GetDisplayName(final_item->GetRecord(), show_names).c_str());
            }

            else if( difference.type == DictionaryDifference::Type::ItemMovedToDifferentRecordOccurrencesDecreased )
            {
                difference_text = FormatText("Record occurrences decreased as a result of movement to a different record (%d -> %d)",
                                             static_cast<int>(initial_item->GetRecord()->GetMaxRecs()),
                                             static_cast<int>(final_item->GetRecord()->GetMaxRecs()));
            }

            else if( difference.type == DictionaryDifference::Type::ItemItemSubitemOccurrencesDeceased ||
                     difference.type == DictionaryDifference::Type::ItemItemSubitemOccurrencesIncreased )
            {
                difference_text = FormatText("Item/subitem occurrences changed (%d -> %d)",
                                             static_cast<int>(initial_item->GetItemSubitemOccurs()),
                                             static_cast<int>(final_item->GetItemSubitemOccurs()));
            }

            else
            {
                ASSERT(false);
            }
        }


        // display the name/label only if is different from the previous one
        std::string display_name;

        if( previous_difference == nullptr || !SO::EqualsNoCase(previous_difference->GetDisplayName(true), difference.GetDisplayName(true)) )
            display_name = difference.GetDisplayName(show_names, display_name_length);

        const BasicLogger::Color color = difference.IsDestructive() ? BasicLogger::Color::Red :
                                                                      BasicLogger::Color::Black;
        differences_log.AppendFormatLine(color, difference_formatter.c_str(), display_name.c_str(), difference_text.c_str());

        previous_difference = &difference;
    }

    if( differences.empty() )
    {
        differences_log.Append("The dictionaries are identical.");
    }

    else if( previous_difference == nullptr )
    {
        differences_log.Append("Reformatting the data will not result in any destructive changes.");
    }
}


std::vector<DictionaryDifference> ToolReformatter::GetSortedDifferences(const Reformatter& reformatter)
{
    std::vector<DictionaryDifference> differences = reformatter.GetDifferences();

    // sort the differences so that differences are displayed as: levels, records, and then items on a record basis, with
    // removals appearing before additions

    // the sort text will be:
    // - a character for the dictionary element type
    // - the level number (for records and items)
    // - the record name (for items)
    // - a character for whether or not it is a removal or an addition
    // - the name
    auto get_sort_text = [](const DictionaryDifference& difference)
    {
        std::string sort_text = difference.IsLevelDifference()  ? "1" :
                                difference.IsRecordDifference() ? "2" :
                                                                  "3";

        if( difference.IsRecordDifference() )
            sort_text.append(IntToString(assert_cast<const CDictRecord*>(difference.GetDefinedDictElement())->GetLevel()->GetLevelNumber()));

        if( difference.IsItemDifference() )
        {
            const CDictRecord* dict_record = assert_cast<const CDictItem*>(difference.GetDefinedDictElement())->GetRecord();
            sort_text.append(IntToString(dict_record->GetLevel()->GetLevelNumber()));
            sort_text.append(dict_record->GetName());
        }

        sort_text.push_back(( difference.final_dict_element == nullptr )   ? '1' :
                            ( difference.initial_dict_element == nullptr ) ? '2' :
                                                                             '3');

        sort_text.append(difference.GetDisplayName(true));

        return sort_text;
    };

    std::sort(differences.begin(), differences.end(),
        [&](const DictionaryDifference& difference1, const DictionaryDifference& difference2)
        {
            const int sort_text_difference = get_sort_text(difference1).compare(get_sort_text(difference2));

            if( sort_text_difference != 0 )
            {
                return ( sort_text_difference < 0 );
            }

            // if the sort text is the same, sort by the type value
            else
            {
                return ( difference1.type < difference2.type );
            }
        });

    return differences;
}


bool ToolReformatter::Run(const PFF& pff, const bool silent, Reformatter& reformatter)
{
    return Run(pff, silent, [&]()
    {
        return &reformatter;
    });
}


bool ToolReformatter::Run(const PFF& pff, const bool silent, std::shared_ptr<const CDataDict> input_dictionary/* = nullptr*/,
                          std::shared_ptr<const CDataDict> output_dictionary/* = nullptr*/)
{
    return Run(pff, silent, [&]()
    {
        // open the dictionaries (if not already open)
        if( input_dictionary == nullptr && !pff.GetInputDictFName().IsEmpty() )
            input_dictionary = CDataDict::InstantiateAndOpen(pff.GetInputDictFName(), silent);

        if( output_dictionary == nullptr )
            output_dictionary = CDataDict::InstantiateAndOpen(pff.GetOutputDictFName(), silent);

        // if the input dictionary hasn't been set, get it from the input data source
        if( input_dictionary == nullptr && pff.GetSingleInputDataConnectionString().IsDefined() )
            input_dictionary = DictionarySource::GetEmbeddedDictionary(pff.GetSingleInputDataConnectionString());

        if( input_dictionary == nullptr )
            throw CSProException("You must specify an input dictionary or an input data source with an embedded dictionary.");

        return std::make_unique<Reformatter>(std::move(input_dictionary), std::move(output_dictionary));
    });
}


template<typename GetReformatterCallback>
bool ToolReformatter::Run(const PFF& pff, const bool /*silent*/, const GetReformatterCallback get_reformatter_callback)
{
    //  open the log file
    if( pff.GetListingFName().IsEmpty() )
        throw CSProException("You must specify a listing file.");

    FileIO::TextFile log;
    log.OpenForTextWritingCreate(pff.GetListingFName());

    // start the reformat
    bool reformat_errors = false;

    try
    {
        cs::non_null_shared_or_raw_ptr<Reformatter> reformatter = get_reformatter_callback();

        // write the listing file header and the dictionary differences
        size_t display_name_length = 0;

        auto write_listing = [&](const DataRepository* const input_repository, const DataRepository* const output_repository)
        {
            if( !pff.GetInputDictFName().IsEmpty() )
            {
                log.WriteFormattedLine("Input Dictionary File:   %s",
                                       UTF8_TODO::GetUtf8(pff.GetInputDictFName()).c_str());
            }

            else
            {
                log.WriteFormattedLine("Input Dictionary File:   %s (Embedded)",
                                       pff.GetSingleInputDataConnectionString().GetFilePath().c_str());
            }

            if( input_repository != nullptr )
            {
                log.WriteFormattedLine("Input Data:              %s",
                                       input_repository->GetName(DataRepositoryNameType::ForListing).c_str());
            }

            log.WriteLine();

            log.WriteFormattedLine("Output Dictionary File:  %s",
                                   UTF8_TODO::GetUtf8(pff.GetOutputDictFName()).c_str());

            if( output_repository != nullptr )
            {
                log.WriteFormattedLine("Output Data:             %s",
                                       output_repository->GetName(DataRepositoryNameType::ForListing).c_str());
            }

            log.WriteLine();

            BasicLogger differences_log;
            GetDictionaryDifferences(differences_log, *reformatter, pff.GetDisplayNames(), false, &display_name_length);
            log.WriteLine(differences_log.ToString());
        };

        // if no output data is specified, just compare the dictionaries
        if( !pff.GetSingleOutputDataConnectionString().IsDefined() )
        {
            write_listing(nullptr, nullptr);
        }

        // otherwise create the case access and open the input and output repositories
        else
        {
            if( !pff.GetSingleInputDataConnectionString().IsDefined() )
                throw CSProException("You must specify an input data source.");

            if( pff.GetSingleInputDataConnectionString().SharesResource(pff.GetSingleOutputDataConnectionString()) )
                throw CSProException("The output data source must be different than the input data source.");

            std::shared_ptr<CaseAccess> input_case_access;
            std::shared_ptr<CaseAccess> output_case_access;
            std::tie(input_case_access, output_case_access) = reformatter->Initialize();

            const std::shared_ptr<ProcessSummary> process_summary = input_case_access->GetDataDict().CreateProcessSummary();
            const auto case_construction_reporter = std::make_shared<StdioCaseConstructionReporter>(log, process_summary);

            input_case_access->SetCaseConstructionReporter(case_construction_reporter);
            output_case_access->SetCaseConstructionReporter(case_construction_reporter);

            const std::unique_ptr<DataRepository> input_repository = DataRepository::CreateAndOpen(input_case_access,
                                                                                                   pff.GetSingleInputDataConnectionString(),
                                                                                                   DataRepositoryAccess::BatchInput,
                                                                                                   DataRepositoryOpenFlag::OpenMustExist);

            const std::unique_ptr<DataRepository> output_repository = DataRepository::CreateAndOpen(output_case_access,
                                                                                                    pff.GetSingleOutputDataConnectionString(),
                                                                                                    DataRepositoryAccess::BatchOutput,
                                                                                                    DataRepositoryOpenFlag::CreateNew);

            const std::unique_ptr<Case> input_case = input_case_access->CreateCase(true);
            const std::unique_ptr<Case> output_case = output_case_access->CreateCase(true);

            write_listing(input_repository.get(), output_repository.get());

            std::map<std::string, std::tuple<const CDictRecord*, size_t, size_t>> records_read_written;

            // show a progress bar while reformatting the data
            ProcessSummaryDlg process_summary_dlg;

            process_summary_dlg.SetTask([&]
            {
                process_summary_dlg.Initialize("Reformatting data...", process_summary);
                process_summary_dlg.SetSource("Input Data: " + input_repository->GetName(DataRepositoryNameType::Full));

                size_t progress_bar_update_counter = ProgressBarCaseUpdateFrequency;

                // keep track of records read and written
                auto update_records_read_written = [&records_read_written](const Case& data_case, const bool read)
                {
                    for( const CaseLevel* const case_level : data_case.GetAllCaseLevels() )
                    {
                        for( size_t record_number = 0; record_number < case_level->GetNumberCaseRecords(); ++record_number )
                        {
                            const CaseRecord& case_record = case_level->GetCaseRecord(record_number);

                            if( case_record.GetNumberOccurrences() > 0 )
                            {
                                const std::string& record_name = case_record.GetCaseRecordMetadata().GetDictRecord().GetName();
                                auto map_search = records_read_written.find(record_name);

                                std::tuple<const CDictRecord*, size_t, size_t>& record_counts =
                                    ( map_search == records_read_written.end() ) ? records_read_written.try_emplace(record_name, &case_record.GetCaseRecordMetadata().GetDictRecord(), 0, 0).first->second :
                                                                                   map_search->second;

                                size_t& record_counter = read ? std::get<1>(record_counts) :
                                                                std::get<2>(record_counts);
                                record_counter += case_record.GetNumberOccurrences();
                            }
                        }
                    }
                };


                // read the cases and do the reformatting
                const std::unique_ptr<CaseIterator> input_case_iterator = input_repository->CreateCaseIterator(CaseIterationMethod::SequentialOrder,
                                                                                                               CaseIterationOrder::Ascending);

                while( input_case_iterator->NextCase(*input_case) )
                {
                    update_records_read_written(*input_case, true);

                    reformatter->ReformatCase(*input_case, *output_case);

                    update_records_read_written(*output_case, false);

                    // warn when the key changes
                    if( input_case->GetKey() != output_case->GetKey() )
                    {
                        log.WriteFormattedLine("*** [%s]\n*** Key changed during reformat: '%s'\n",
                                               NewlineSubstitutor::NewlineToUnicodeNL(input_case->GetKey()).c_str(),
                                               NewlineSubstitutor::NewlineToUnicodeNL(output_case->GetKey()).c_str());
                    }

                    output_repository->WriteCase(*output_case);

                    // update the progress bar
                    if( process_summary_dlg.IsCanceled() )
                        throw UserCanceledException();

                    if( --progress_bar_update_counter == 0 )
                    {
                        process_summary->SetPercentSourceRead(input_case_iterator->GetPercentRead());
                        process_summary_dlg.SetKey(input_case->GetKey());
                        progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
                    }
                }
            });

            process_summary_dlg.DoModal();

            process_summary_dlg.RethrowTaskExceptions();


            // count any bad records as reformat errors
            reformat_errors |= ( case_construction_reporter->GetBadRecordCount() > 0 );

            // write out the record summary
            log.WriteLine("Records processed:");

            log.WriteFormattedLine("    %*s%10s%10s",
                                   static_cast<int>(display_name_length), "",
                                   "Input", "Output");

            for( const auto& [record_name, record_counts] : records_read_written )
            {
                const std::string display_name = DictionaryDifference::GetDisplayName(std::get<0>(record_counts), pff.GetDisplayNames(), display_name_length);
                log.WriteFormattedLine("    %-*s%10d%10d",
                                       static_cast<int>(display_name_length), display_name.c_str(),
                                       static_cast<int>(std::get<1>(record_counts)), static_cast<int>(std::get<2>(record_counts)));
            }
        }
    }

    catch( const CSProException& exception )
    {
        reformat_errors = true;
        log.WriteLine();
        log.WriteLine(exception.what());
    }

    // close the log and potentially view the listing and results
    log.Close();

    if( pff.GetViewListing() == ALWAYS || ( pff.GetViewListing() == ONERROR && reformat_errors ) )
        pff.ViewListing();

    if( pff.GetViewResultsFlag() && !reformat_errors )
        PFF::ViewResults(pff.GetSingleOutputDataConnectionString());

    return !reformat_errors;
}
