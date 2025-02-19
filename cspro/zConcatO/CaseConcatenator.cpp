#include "stdafx.h"
#include "CaseConcatenator.h"
#include "ConcatenatorHelpers.h"
#include "ConcatenatorReporter.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/ExpansiveMap.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepository.h>
#include <zDataO/DataRepositoryHelpers.h>


namespace
{
    constexpr size_t ProgressBarCaseUpdateFrequency = 100;
}


void CaseConcatenator::Run(ConcatenatorReporter& concatenator_reporter,
                           const std::vector<ConnectionString>& input_connection_strings, const ConnectionString& output_connection_string,
                           std::shared_ptr<const CDataDict> dictionary, std::shared_ptr<CaseConstructionReporter> case_construction_reporter)
{
    // calculate the size of each data source, estimating URL-based data sources as having 5 mb of data
    constexpr uint64_t EstimatedFileSizeForUrls = 5 * 1024 * 1024;
    std::vector<std::optional<uint64_t>> file_sizes;
    uint64_t total_file_size;
    std::tie(file_sizes, total_file_size) = ConcatenatorHelpers::CalculateFileSizes(input_connection_strings, EstimatedFileSizeForUrls);

    // for file-based data sources, output to a temporary file so that an input data source can also be an output data source
    std::unique_ptr<const ConnectionString> temporary_output_connection_string;

    if( output_connection_string.HasFilePath() )
    {
        temporary_output_connection_string = std::make_unique<ConnectionString>(PathHelpers::AppendToConnectionStringFilename(output_connection_string, "~temp~"));
    }

    // otherwise make sure the output data source differs from the input data source
    else if( output_connection_string.SharesResource(input_connection_strings) )
    {
        throw CSProException("Output data source '%s' is also one of the data sources to concatenate. "
                             "This is not possible for data sources of type '%s'.",
                             output_connection_string.ToDisplayString().c_str(),
                             ToString(output_connection_string.GetType()));
    }

    const std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);
    case_access->SetCaseConstructionReporter(case_construction_reporter);

    const std::unique_ptr<Case> data_case = case_access->CreateCase(true);

    const std::unique_ptr<DataRepository> output_repository =
        DataRepository::CreateAndOpen(case_access,
                                      ( temporary_output_connection_string != nullptr ) ? *temporary_output_connection_string : output_connection_string,
                                      DataRepositoryAccess::BatchOutput,
                                      DataRepositoryOpenFlag::CreateNew);

    // concatenate each data source
    ExpansiveMap<std::string, size_t> key_duplicate_checker;
    ExpansiveMap<std::string, size_t> uuid_duplicate_checker;

    double progress_bar_value = 0;
    size_t progress_bar_update_counter = ProgressBarCaseUpdateFrequency;

    try
    {
        for( size_t i = 0; i < input_connection_strings.size(); ++i )
        {
            if( concatenator_reporter.IsCanceled() )
                throw UserCanceledException();

            const ConnectionString& input_connection_string = input_connection_strings[i];
            const std::optional<uint64_t>& file_size = file_sizes[i];

            // open the data source
            std::unique_ptr<DataRepository> input_repository;

            try
            {
                // skip data sources that don't exist
                if( !file_sizes[i].has_value() )
                    throw FileIO::Exception::FileNotFound(input_connection_string.ToDisplayString());

                input_repository = DataRepository::CreateAndOpen(case_access, input_connection_string,
                                                                 DataRepositoryAccess::BatchInput,
                                                                 DataRepositoryOpenFlag::OpenMustExist);
            }

            catch( const CSProException& exception )
            {
                concatenator_reporter.ErrorDataSourceOpenFailed(input_connection_string, exception.what());
                continue;
            }

            double initial_progress_bar_value = progress_bar_value;
            const double this_data_source_progress_bar_proportion = CreatePercent<double>(*file_size, total_file_size) / 100;

            // read and write the cases
            concatenator_reporter.SetSource(ConcatenatorHelpers::GetReporterSourceText("Data Source", input_connection_strings, i));

            try
            {
                std::unique_ptr<CaseIterator> case_iterator = input_repository->CreateCaseIterator(CaseIterationMethod::SequentialOrder,
                                                                                                   CaseIterationOrder::Ascending);

                while( case_iterator->NextCase(*data_case) )
                {
                    // only write the case if it is not a duplicate
                    cs::shared_or_raw_ptr<const size_t> previous_data_source_number = key_duplicate_checker.Find(data_case->GetKey());

                    if( previous_data_source_number == nullptr && !data_case->GetUuid().empty() )
                        previous_data_source_number = uuid_duplicate_checker.Find(data_case->GetUuid());

                    if( previous_data_source_number != nullptr )
                    {
                        concatenator_reporter.ErrorDuplicateCase(data_case->GetKey(), input_connection_string, input_connection_strings[*previous_data_source_number]);
                    }

                    else
                    {
                        output_repository->WriteCase(*data_case);

                        key_duplicate_checker.Insert(data_case->GetKey(), i);

                        if( !data_case->GetUuid().empty() )
                            uuid_duplicate_checker.Insert(data_case->GetUuid(), i);
                    }

                    // check for cancelation and update the progress bar
                    if( concatenator_reporter.IsCanceled() )
                        throw UserCanceledException();

                    if( --progress_bar_update_counter == 0 )
                    {
                        progress_bar_value = initial_progress_bar_value + case_iterator->GetPercentRead() * this_data_source_progress_bar_proportion;
                        concatenator_reporter.GetProcessSummary().SetPercentSourceRead(progress_bar_value);
                        concatenator_reporter.SetKey(data_case->GetKey());
                        progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
                    }
                }

                case_iterator.reset();
                input_repository->Close();

                concatenator_reporter.AddSuccessfullyProcessedTarget(input_connection_string);
            }

            catch( const DataRepositoryException::Error& exception )
            {
                concatenator_reporter.ErrorOther(input_connection_string, exception.what());
            }
        }

        output_repository->Close();
    }

    catch(...)
    {
        if( output_repository != nullptr )
            output_repository->DeleteRepository();

        throw;
    }

    // after a success concatenation, rename the temporary file
    if( temporary_output_connection_string != nullptr )
        DataRepositoryHelpers::RenameRepository(*temporary_output_connection_string, output_connection_string);
}
