#include "stdafx.h"
#include "CaseConcatenator.h"
#include "ConcatenatorHelpers.h"
#include "ConcatenatorReporter.h"
#include "DuplicateCaseChecker.h"
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
    // calculate the size of each file
    std::vector<std::optional<uint64_t>> file_sizes;
    uint64_t total_file_size;
    std::tie(file_sizes, total_file_size) = ConcatenatorHelpers::CalculateFileSizes(input_connection_strings);

    // create the output file using a temporary filename
    const ConnectionString temporary_output_connection_string = ConcatenatorHelpers::GetTemporaryOutputConnectionString(output_connection_string);

    const std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);
    case_access->SetCaseConstructionReporter(case_construction_reporter);

    const std::unique_ptr<Case> data_case = case_access->CreateCase(true);

    std::unique_ptr<DataRepository> output_repository = DataRepository::CreateAndOpen(case_access,
                                                                                      temporary_output_connection_string,
                                                                                      DataRepositoryAccess::BatchOutput,
                                                                                      DataRepositoryOpenFlag::CreateNew);

    // concatenate each data file
    DuplicateCaseChecker key_duplicate_checker;
    DuplicateCaseChecker uuid_duplicate_checker;

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

            // skip files that don't exist
            if( !file_sizes[i].has_value() )
            {
                concatenator_reporter.ErrorFileOpenFailed(input_connection_string);
                continue;
            }

            // open the file
            std::unique_ptr<DataRepository> input_repository;

            try
            {
                input_repository = DataRepository::CreateAndOpen(case_access, input_connection_string,
                                                                 DataRepositoryAccess::BatchInput,
                                                                 DataRepositoryOpenFlag::OpenMustExist);
            }

            catch( const DataRepositoryException::Error& )
            {
                concatenator_reporter.ErrorFileOpenFailed(input_connection_string);
                continue;
            }

            double initial_progress_bar_value = progress_bar_value;
            const double this_file_progress_bar_proportion = CreatePercent<double>(*file_size, total_file_size) / 100;

            // read and write the cases
            concatenator_reporter.SetSource(ConcatenatorHelpers::GetReporterSourceText(input_connection_strings, i));

            try
            {
                std::unique_ptr<CaseIterator> case_iterator = input_repository->CreateCaseIterator(CaseIterationMethod::SequentialOrder,
                                                                                                   CaseIterationOrder::Ascending);

                while( case_iterator->NextCase(*data_case) )
                {
                    // only write the case if it is not a duplicate
                    std::optional<size_t> previous_file_number = key_duplicate_checker.LookupDuplicate(data_case->GetKey());

                    if( !previous_file_number.has_value() && !data_case->GetUuid().IsEmpty() )
                        previous_file_number = uuid_duplicate_checker.LookupDuplicate(data_case->GetUuid());

                    if( previous_file_number.has_value() )
                    {
                        concatenator_reporter.ErrorDuplicateCase(data_case->GetKey(), input_connection_string, input_connection_strings[*previous_file_number]);
                    }

                    else
                    {
                        output_repository->WriteCase(*data_case);

                        key_duplicate_checker.Add(data_case->GetKey(), i);

                        if( !data_case->GetUuid().IsEmpty() )
                            uuid_duplicate_checker.Add(data_case->GetUuid(), i);
                    }

                    // check for cancelation and update the progress bar
                    if( concatenator_reporter.IsCanceled() )
                        throw UserCanceledException();

                    if( --progress_bar_update_counter == 0 )
                    {
                        progress_bar_value = initial_progress_bar_value + case_iterator->GetPercentRead() * this_file_progress_bar_proportion;
                        concatenator_reporter.GetProcessSummary().SetPercentSourceRead(progress_bar_value);
                        concatenator_reporter.SetKey(data_case->GetKey());
                        progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
                    }
                }

                case_iterator.reset();
                input_repository->Close();

                concatenator_reporter.AddSuccessfullyProcessedFile(input_connection_string);
            }

            catch( const DataRepositoryException::Error& exception )
            {
                concatenator_reporter.ErrorOther(input_connection_string, exception.GetErrorMessage());
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

    // after a success concatenation, rename the temporary file to the output filename
    DataRepositoryHelpers::RenameRepository(temporary_output_connection_string, output_connection_string);
}
