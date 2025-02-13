#include "Stdafx.h"
#include "RepositoryConverter.h"
#include "DataViewerCaseConstructionReporter.h"


System::String^ CSPro::Data::RepositoryConverter::Convert(System::ComponentModel::BackgroundWorker^ background_worker,
    CSPro::Data::DataRepository^ input_repository, CSPro::Util::ConnectionString^ output_connection_string)
{
    try
    {
        auto native_input_repository = input_repository->GetNativePointer();

        auto case_access = native_input_repository->GetSharedCaseAccess();

        // use a case construction reporter to keep track of any errors on conversion
        auto case_construction_reporter = std::make_shared<DataViewerCaseConstructionReporter>();
        const_cast<CaseAccess&>(*case_access).SetCaseConstructionReporter(case_construction_reporter);

        auto data_case = case_access->CreateCase(true);
        
        std::unique_ptr<::DataRepository> output_repository(::DataRepository::CreateAndOpen(case_access,
            output_connection_string->GetNativeConnectionString(), DataRepositoryAccess::BatchOutput, DataRepositoryOpenFlag::CreateNew));

        background_worker->ReportProgress(-1, System::String::Format("Saving {0} to {1}",
            gcnew System::String(native_input_repository->GetName(DataRepositoryNameType::Concise)),
            gcnew System::String(output_repository->GetName(DataRepositoryNameType::Concise))));

        size_t total_cases = native_input_repository->GetNumberCases();
        size_t converted_cases = 0;

        const size_t ProgressUpdateFrequency = 250;
        size_t cases_until_progress_update = ProgressUpdateFrequency;
        double percent_multiplier = 100.0 / std::max(total_cases, (size_t)1);

        auto case_iterator = native_input_repository->CreateCaseIterator(::CaseIterationMethod::SequentialOrder, ::CaseIterationOrder::Ascending);

        while( case_iterator->NextCase(*data_case) )
        {
            output_repository->WriteCase(*data_case);

            ++converted_cases;

            if( --cases_until_progress_update == 0 )
            {
                // update progress bar
                background_worker->ReportProgress((int)( converted_cases * percent_multiplier ),
                    System::String::Format("Case {0} of {1} [{2}]", converted_cases, total_cases, gcnew System::String(data_case->GetKey())));

                cases_until_progress_update = ProgressUpdateFrequency;
            }

            // if the operation is cancelled, delete the output repository
            if( background_worker->CancellationPending )
            {
                output_repository->DeleteRepository();
                return nullptr;
            }
        }

        background_worker->ReportProgress(100, gcnew System::String("Conversion complete"));

        // update the list of converted cases (in case some were not output)
        if( DataRepositoryHelpers::TypeSupportsIndexedQueriesInBatchOutput(output_connection_string->GetNativeConnectionString().GetType()) )
            converted_cases = output_repository->GetNumberCases();

        CString message = FormatText(_T("Successfully saved %d cases."), (int)converted_cases);

        if( converted_cases != total_cases )
            message.AppendFormat(_T(" %d cases were not output (probably because they were duplicate cases)."), (int)( total_cases - converted_cases ));

        if( !case_construction_reporter->GetErrors().empty() )
        {
            message.Append(_T("\n\nThere were errors during the conversion:"));

            for( const std::wstring& error : case_construction_reporter->GetErrors() )
                message.AppendFormat(_T("\n\n• %s"), error.c_str());
        }

        return gcnew System::String(message);
    }

    catch( const DataRepositoryException::Error& exception )
    {
        return gcnew System::String(exception.GetErrorMessage().c_str());
    }
}
