//---------------------------------------------------------------------------
//
// EXOPFILE.cpp   : opening and closing of external files
//
//---------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "Engine.h"
#include "BinaryStorageFor80.h"
#include <zEngineO/EngineCaseConstructionReporter.h>
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/LoopStack.h>
#include <zDictO/DDClass.h>
#include <zDataO/CacheableCaseWrapperRepository.h>
#include <zDataO/ConnectionStringProperties.h>
#include <zDataO/DataRepository.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zDataO/ParadataWrapperRepository.h>
#include <zParadataO/Logger.h>
#include <ZBRIDGEO/npff.h>


void CEngineDriver::InitializeData()
{
    // because multiple dictionaries may be using the same case access, determine the greatest status required
    std::set<CaseAccess*> case_accesses;

    for( const EngineDictionary* engine_dictionary : m_engineData->engine_dictionaries )
    {
        if( !engine_dictionary->HasEngineDataRepository() )
            continue;

        const EngineDataRepository& engine_data_repository = engine_dictionary->GetEngineDataRepository();

        CaseAccess* case_access = engine_dictionary->GetCaseAccess();
        case_accesses.emplace(case_access);

        // full access is required of entry inputs, batch inputs when there is an output,
    	// special outputs, and writeable external dictionaries
        bool set_requires_full_access = false;

        switch( engine_dictionary->GetSubType() )
        {
            case SymbolSubType::Input:
            {
                if( Appl.ApplicationType == ModuleType::Entry )
                {
                    set_requires_full_access = true;
                }

                else
                {
                    if( m_pPifFile->UsingOutputData() )
                    {
                        set_requires_full_access = true;
                    }

                    // if running an entry application in Run As Batch mode, all dictionary items need to be read in
                    else if( m_pEngineSettings->HasCheckRanges() || m_pEngineSettings->HasSkipStruc() )
                    {
                        case_access->SetUseAllDictionaryItems();
                    }
                }

                break;
            }

            case SymbolSubType::Output:
            {
                set_requires_full_access = true;
                break;
            }

            case SymbolSubType::External:
            {
                set_requires_full_access = engine_data_repository.GetIsWriteable();
                break;
            }

            default:
            {
                ASSERT(engine_dictionary->GetSubType() == SymbolSubType::Work);
            }
        }

        if( set_requires_full_access )
            case_access->SetRequiresFullAccess();
    }


    // create a default case construction reporter
    auto default_case_construction_reporter = std::make_shared<EngineCaseConstructionReporter>(
        m_pEngineDriver->GetSharedSystemMessageIssuer(), nullptr);

    // initialize the case accesses
    for( CaseAccess* case_access : case_accesses )
    {
        ASSERT(false); // 20220810 note: when this code becomes active, make sure that this method is only called after
                       // the File Associations dialog is dismissed, otherwise the call to m_pPifFile->UsingOutputData()
                       // above will not be accurate; alternatively, keep it here but make sure the File Associations dialog
                       // is presented before this method is called;
                       // if doing the latter (the better option), then the listers's approach to working with the CaseAccess object
                       // can be refactored as the object will have been initialized prior to the lister's constructor being called

        case_access->SetCaseConstructionReporter(default_case_construction_reporter);
        case_access->Initialize();
    }


    // initialize the dictionaries
    for( EngineDictionary* engine_dictionary : m_engineData->engine_dictionaries )
    {
        std::shared_ptr<EngineCaseConstructionReporter> case_construction_reporter_override;

#ifdef WIN_DESKTOP
        // create the batch input case construction reporter
        if( engine_dictionary->GetSubType() == SymbolSubType::Input && Appl.ApplicationType != ModuleType::Entry )
        {
            std::function<void(const Case&)> update_case_callback = [&](const Case& data_case)
            {
                m_pEngineDriver->GetLister()->SetMessageSource(data_case);
            };

            // we will only use a process summary for the batch input file so that record counts for external dictionaries don't get added in
            case_construction_reporter_override = std::make_shared<EngineCaseConstructionReporter>(
                m_pEngineDriver->GetSharedSystemMessageIssuer(), m_pEngineDriver->GetProcessSummary(), update_case_callback);
        }
#endif

        std::shared_ptr<std::function<void(EngineDataRepository&)>> reset_override;

        if( engine_dictionary->IsDataRepositoryObject() )
        {
            // this reset override is handled this way because the EngineDictionary symbol
            // does not have access to CEngineDriver methods
            reset_override = std::make_shared<std::function<void(EngineDataRepository&)>>(
                [&](EngineDataRepository& engine_data_repository)
                {
                    m_pEngineDriver->LoadAllBinaryDataFromRepository(engine_data_repository);
                });
        }

        engine_dictionary->InitializeRuntime(m_pEngineDriver->GetSharedSystemMessageIssuer(),
            case_construction_reporter_override, reset_override);
    }
}



// opens the repository using the characteristics of the dictionary; throws DataRepositoryException exceptions
void CEngineDriver::OpenRepository(EngineDataRepository& engine_data_repository, const ConnectionString& connection_string,
                                   DataRepositoryOpenFlag open_flag, bool load_binary_data_from_currently_open_repository_before_closing)
{
    const EngineDictionary& engine_dictionary = engine_data_repository.GetEngineDictionary();

    // end any transactions (from operations in a loop) and then close the current repository
    m_pIntDriver->GetLoopStack().RemoveDataTransactionFromLoopStack(engine_data_repository.GetDataRepository());

    if( load_binary_data_from_currently_open_repository_before_closing )
        LoadAllBinaryDataFromRepository(engine_data_repository);

    engine_data_repository.CloseDataRepository();


    DataRepositoryAccess access_type = DataRepositoryAccess::ReadOnly;

    // input dictionaries
    if( engine_dictionary.GetSubType() == SymbolSubType::Input )
    {
        access_type = ( Appl.ApplicationType == ModuleType::Entry ) ? DataRepositoryAccess::EntryInput :
                                                                      DataRepositoryAccess::BatchInput;
    }

    // external dictionaries
    else if( engine_dictionary.GetSubType() == SymbolSubType::External )
    {
        if( engine_data_repository.GetIsWriteable() )
            access_type = DataRepositoryAccess::ReadWrite;

        if( engine_data_repository.GetCannotHaveIndex() )
            access_type = DataRepositoryAccess::BatchOutputAppend;
    }

    // special output dictionaries
    else if( engine_dictionary.GetSubType() == SymbolSubType::Output )
    {
        access_type = DataRepositoryAccess::BatchOutput;
    }

    else
    {
        ASSERT(false);
    }

    std::shared_ptr<DataRepository> data_repository = DataRepository::Create(engine_dictionary.GetSharedCaseAccess(), connection_string, access_type);

    // wrap the repository if using the paradata
    if( Paradata::Logger::IsOpen() )
    {
        data_repository = std::make_shared<ParadataWrapperRepository>(std::move(data_repository), access_type,
            *m_pIntDriver->m_pParadataDriver, m_pIntDriver->m_pParadataDriver->CreateObject(engine_dictionary));
    }

    // see if the cases should be cached
    if( connection_string.HasProperty(CSProperty::cache, CSValue::true_, true) )
        data_repository = CacheableCaseWrapperRepository::CreateCacheableCaseWrapperRepository(std::move(data_repository), access_type);

    data_repository->Open(connection_string, open_flag);

    engine_data_repository.SetDataRepository(std::move(data_repository));

    // set the iteration method
    DictionaryAccessParameters dictionary_access_parameters = engine_data_repository.GetDictionaryAccessParameters();
    dictionary_access_parameters.case_iteration_method = ( access_type != DataRepositoryAccess::BatchInput || m_pPifFile->GetInputOrderIndexed() ) ?
        CaseIterationMethod::KeyOrder : CaseIterationMethod::SequentialOrder;
    engine_data_repository.SetDictionaryAccessParameters(dictionary_access_parameters);

    // if batch input, start the iterator and update the listing
    if( access_type == DataRepositoryAccess::BatchInput )
    {
        engine_data_repository.CreateCaseIterator(EngineDataRepository::CaseIteratorStyle::FromBoundary);
        m_lister->UpdateCaseSourceDetails(connection_string, engine_dictionary.GetDictionary());
    }

    if( engine_data_repository.GetUsesSync() && !engine_data_repository.GetHasDynamicFileManagement() )
    {
        if( !DataRepositoryHelpers::TypeSupportsSync(engine_data_repository.GetDataRepository().GetRepositoryType()) )
            issaerror(MessageType::Warning, 10108, engine_dictionary.GetName().c_str());
    }
}


void CEngineDriver::OpenRepository(DICX* pDicX, const ConnectionString& connection_string,
                                   DataRepositoryOpenFlag eOpenFlag, bool load_binary_data_from_currently_open_repository_before_closing)
{
    DICT* pDicT = pDicX->GetDicT();

    // end any transactions (from operations in a loop) and then close the current repository
    if( pDicX->IsDataRepositoryOpen() )
        m_pIntDriver->GetLoopStack().RemoveDataTransactionFromLoopStack(pDicX->GetDataRepository());

    if( load_binary_data_from_currently_open_repository_before_closing )
        LoadAllBinaryDataFromRepository(pDicX);

    pDicX->CloseDataRepository();

    DataRepositoryAccess access_type = DataRepositoryAccess::ReadOnly;

    // input dictionaries
    if( pDicT->GetSubType() == SymbolSubType::Input )
    {
        access_type = ( Appl.ApplicationType == ModuleType::Entry ) ? DataRepositoryAccess::EntryInput : DataRepositoryAccess::BatchInput;
    }

    // external dictionaries
    else if( pDicT->GetSubType() == SymbolSubType::External )
    {
        if( pDicT->GetWriteable() )
            access_type = DataRepositoryAccess::ReadWrite;

        if( pDicT->GetCannotHaveIndex() )
            access_type = DataRepositoryAccess::BatchOutputAppend;
    }

    // special output dictionaries
    else if( pDicT->GetSubType() == SymbolSubType::Output )
    {
        access_type = DataRepositoryAccess::BatchOutput;
    }

    else
    {
        ASSERT(false);
    }

    std::shared_ptr<DataRepository> data_repository = DataRepository::Create(pDicT->GetSharedCaseAccess(), connection_string, access_type);

    // wrap the repository if using the paradata
    if( Paradata::Logger::IsOpen() )
    {
        data_repository = std::make_shared<ParadataWrapperRepository>(std::move(data_repository), access_type,
            *m_pIntDriver->m_pParadataDriver, m_pIntDriver->m_pParadataDriver->CreateObject(*pDicT));
    }

    // see if the cases should be cached
    if( connection_string.HasProperty(CSProperty::cache, CSValue::true_, true) )
        data_repository = CacheableCaseWrapperRepository::CreateCacheableCaseWrapperRepository(std::move(data_repository), access_type);

    pDicX->SetDataRepository(std::move(data_repository));

    pDicX->GetDataRepository().Open(connection_string, eOpenFlag);

    // set the iteration method
    CaseIterationMethod iteration_method = ( access_type != DataRepositoryAccess::BatchInput || m_pPifFile->GetInputOrderIndexed() ) ?
        CaseIterationMethod::KeyOrder : CaseIterationMethod::SequentialOrder;
    pDicX->SetCaseIterationMethod(iteration_method);

    // if batch input, start the iterator and update the listing
    if( access_type == DataRepositoryAccess::BatchInput )
    {
        pDicX->CreateCaseIterator(DICX::CaseIteratorStyle::FromBoundary);
        m_lister->UpdateCaseSourceDetails(connection_string, *pDicT->GetDataDict());
    }

    if( pDicT->GetUsesSync() && !pDicT->GetHasDynamicFileManagement() )
    {
        if( !DataRepositoryHelpers::TypeSupportsSync(pDicX->GetDataRepository().GetRepositoryType()) )
            issaerror(MessageType::Warning, 10108, pDicT->GetName().c_str());
    }
}


bool CEngineDriver::OpenBatchOutputRepositories(const std::vector<ConnectionString>& output_connection_strings, bool setoutput_mode)
{
    std::vector<ConnectionString> successfully_opened_connection_strings;

    CloseBatchOutputRepositories();

    for( const ConnectionString& output_connection_string : output_connection_strings )
    {
        try
        {
            DataRepositoryAccess access_type = DataRepositoryAccess::BatchOutput;

            // if there is more than one input file we could have duplicate uuids in the output file;
            // using append mode tells the repository that this is possible so it will handle it correctly
            if( setoutput_mode || m_pPifFile->GetInputDataConnectionStrings().size() > 1 )
                access_type = DataRepositoryAccess::BatchOutputAppend;

            if( UseNewDriver() )
            {
                const EngineDictionary& input_engine_dictionary = *m_engineData->engine_dictionaries.front();

                std::shared_ptr<DataRepository> data_repository = DataRepository::Create(input_engine_dictionary.GetSharedCaseAccess(),
                    output_connection_string, access_type);

                if( Paradata::Logger::IsOpen() )
                {
                    data_repository = std::make_shared<ParadataWrapperRepository>(std::move(data_repository),
                        DataRepositoryAccess::BatchOutput, *m_pIntDriver->m_pParadataDriver,
                        m_pIntDriver->m_pParadataDriver->CreateObject(input_engine_dictionary));
                }

                data_repository->Open(output_connection_string, setoutput_mode ? DataRepositoryOpenFlag::OpenOrCreate :
                                                                                 DataRepositoryOpenFlag::CreateNew);

                m_batchOutputRepositories.emplace_back(std::move(data_repository));
                successfully_opened_connection_strings.emplace_back(output_connection_string);
            }

            else
            {
                std::shared_ptr<DataRepository> data_repository = DataRepository::Create(DIP(0)->GetSharedCaseAccess(),
                    output_connection_string, access_type);

                if( Paradata::Logger::IsOpen() )
                {
                    data_repository = std::make_shared<ParadataWrapperRepository>(std::move(data_repository),
                        DataRepositoryAccess::BatchOutput, *m_pIntDriver->m_pParadataDriver,
                        m_pIntDriver->m_pParadataDriver->CreateObject(*DIP(0)));
                }

                data_repository->Open(output_connection_string, setoutput_mode ? DataRepositoryOpenFlag::OpenOrCreate :
                                                                                 DataRepositoryOpenFlag::CreateNew);

                m_batchOutputRepositories.emplace_back(std::move(data_repository));
                successfully_opened_connection_strings.emplace_back(output_connection_string);
            }
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Error, setoutput_mode ? 29007 : 29005,
                      PortableFunctions::PathGetFilename(output_connection_string.GetFilename()),
                      exception.GetErrorMessage().c_str());
        }
    }

    // update the PFF's output data to reflect what was opened
    if( successfully_opened_connection_strings.empty() )
    {
        m_pEngineDriver->m_pPifFile->SetSingleOutputDataConnectionString(_T(""));
        return false;
    }

    else
    {
        m_pEngineDriver->m_pPifFile->ClearAndAddOutputDataConnectionStrings(successfully_opened_connection_strings);
        return true;
    }
}


void CEngineDriver::CloseBatchOutputRepositories()
{
    for( const std::shared_ptr<DataRepository>& data_repository : m_batchOutputRepositories )
    {
        try
        {
            data_repository->Close();
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Error, 10105, exception.GetErrorMessage().c_str());
        }
    }

    m_batchOutputRepositories.clear();
}


bool CEngineDriver::OpenRepositories(bool open_input_repository)
{
    // open the repositories
    size_t iStartingSlot = open_input_repository ? 0 : 1;
    bool use_null_repository_override = false;

    // open the special output and external files
    for( size_t iDicSlot = iStartingSlot; iDicSlot < m_engineData->dictionaries_pre80.size(); iDicSlot++ )
    {
        DICT* pDicT = DIP(iDicSlot);
        DICX* pDicX = pDicT->GetDicX();

        if( pDicT->GetSubType() == SymbolSubType::Work )
            continue;

        ConnectionString connection_string = ( iDicSlot == 0 ) ? m_pPifFile->GetSingleInputDataConnectionString() :
                                                                 m_pPifFile->GetExternalDataConnectionString(WS2CS(pDicT->GetName()));

        // we are loading the null repository instead of whatever repository failed upon opening
        if( use_null_repository_override || !connection_string.IsDefined() )
        {
            connection_string = ConnectionString::CreateNullRepositoryConnectionString();
            use_null_repository_override = false;
        }

        try
        {
            OpenRepository(pDicX, connection_string, DataRepositoryOpenFlag::OpenOrCreate, false);
        }

        catch( const DataRepositoryException::Error& exception )
        {
            // in entry, instead of fatal errors, repository open errors will be warnings with the repository
            // then set to the null repository; if the error occurred when opening a null repository, then we genuinely
            // have a problem
            bool issue_warning_not_error = ( Appl.ApplicationType == ModuleType::Entry && connection_string.GetType() != DataRepositoryType::Null );

            // there are some exceptions to the above rule, such as if the dictionary format of a CSPro DB file changed,
            // or the password for an Encrypted CSPro DB file is not provided, in which case the program will abort
            if( exception.ForceAbortOnStartup() )
                issue_warning_not_error = false;

            issaerror(issue_warning_not_error ? MessageType::Warning : MessageType::Error, 10102, pDicT->GetName().c_str(), exception.GetErrorMessage().c_str());

            if( issue_warning_not_error )
            {
                issaerror(MessageType::Warning, 10106, pDicT->GetName().c_str());
                use_null_repository_override = true;
                iDicSlot--;
                continue;
            }

            else
            {
                return false;
            }
        }
    }


    for( EngineDictionary* engine_dictionary : m_engineData->engine_dictionaries )
    {
        // don't process Case or DataSource objects
        if( !engine_dictionary->IsDictionaryObject() )
            continue;

        // clear working storage dictionaries (to set all of the records and items to the working state)
        if( engine_dictionary->GetSubType() == SymbolSubType::Work )
        {
            engine_dictionary->GetEngineCase().ClearCase();
            continue;
        }

        // the input file for a batch application is opened in CBatchDriver::RunDriver
        if( engine_dictionary->GetSubType() == SymbolSubType::Input && Appl.ApplicationType != ModuleType::Entry )
            continue;

        ConnectionString connection_string = ( engine_dictionary->GetSubType() == SymbolSubType::Input ) ?
            m_pPifFile->GetSingleInputDataConnectionString() :
            m_pPifFile->GetExternalDataConnectionString(WS2CS(engine_dictionary->GetName()));

        enum class OpenAction { Success, TryAgainWithNullRepository, Abort };
        OpenAction open_action;

        auto open_repository = [&]()
        {
            if( !connection_string.IsDefined() || open_action == OpenAction::TryAgainWithNullRepository )
                connection_string = ConnectionString::CreateNullRepositoryConnectionString();

            try
            {
                OpenRepository(engine_dictionary->GetEngineDataRepository(), connection_string, DataRepositoryOpenFlag::OpenOrCreate, false);
                open_action = OpenAction::Success;
            }

            catch( const DataRepositoryException::Error& exception )
            {
                // in entry, instead of fatal errors, repository open errors will be warnings with the repository
                // then set to the null repository; if the error occurred when opening a null repository, then we genuinely
                // have a problem
                bool issue_warning_not_error = ( Appl.ApplicationType == ModuleType::Entry &&
                                                 connection_string.GetType() != DataRepositoryType::Null );

                // there are some exceptions to the above rule, such as if the dictionary format of a CSPro DB file changed,
                // or the password for an Encrypted CSPro DB file is not provided, in which case the program will abort
                if( exception.ForceAbortOnStartup() )
                    issue_warning_not_error = false;

                issaerror(issue_warning_not_error ? MessageType::Warning : MessageType::Error, 10102,
                          engine_dictionary->GetName().c_str(), exception.GetErrorMessage().c_str());

                if( issue_warning_not_error && open_action != OpenAction::TryAgainWithNullRepository )
                {
                    issaerror(MessageType::Warning, 10106, engine_dictionary->GetName().c_str());
                    open_action = OpenAction::TryAgainWithNullRepository;
                }

                else
                {
                    open_action = OpenAction::Abort;
                }
            }
        };

        open_repository();

        if( open_action == OpenAction::TryAgainWithNullRepository )
            open_repository();

        if( open_action != OpenAction::Success )
            return false;
    }


    return true;
}


void CEngineDriver::LoadAllBinaryDataFromRepository(EngineDataRepository& engine_data_repository)
{
    // because a case loaded from a data repository remains in memory after the repository has been
    // closed, we need to make sure that all binary data, which is lazy loaded, gets loaded
    const CDataDict* this_dictionary = &engine_data_repository.GetEngineDictionary().GetDictionary();

    for( EngineDictionary* engine_dictionary : m_engineData->engine_dictionaries )
    {
        if( !engine_dictionary->HasEngineCase() || this_dictionary != &engine_dictionary->GetDictionary() )
            continue;

        // at this point we know that this case could have come from this data repository
        Case& data_case = engine_dictionary->GetEngineCase().GetCase();

        // quit out if this dictionary has no binary data
        if( !data_case.GetCaseMetadata().UsesBinaryData() )
            return;

        // a future enhancement could make this more efficient and the data repository used to load a case could
        // be stored along with the case object so that binary data isn't loaded unnecessary
        data_case.LoadAllBinaryData();
    }
}


void CEngineDriver::LoadAllBinaryDataFromRepository(DICX* pDicX)
{
    for( const std::shared_ptr<BinaryStorageFor80>& binary_storage : pDicX->GetDicT()->m_binaryStorageFor80 )
    {
        // if the data cannot be loaded, clear it
        if( binary_storage->GetBinaryData_noexcept(pDicX->GetCase()) == nullptr )
            binary_storage->binary_data_accessor.Clear();
    }
}


void CEngineDriver::CloseRepositories(bool close_input_repository)
{
    for( size_t iDicSlot = ( close_input_repository ? 0 : 1 ); iDicSlot < m_engineData->dictionaries_pre80.size(); iDicSlot++ )
    {
        DICT* pDicT = DIP(iDicSlot);
        DICX* pDicX = pDicT->GetDicX();
        pDicX->CloseDataRepository();
    }

    for( EngineDictionary* engine_dictionary : m_engineData->engine_dictionaries )
    {
        if( engine_dictionary->HasEngineDataRepository() &&
            ( close_input_repository || engine_dictionary->GetSubType() != SymbolSubType::Input ) )
        {
            engine_dictionary->GetEngineDataRepository().CloseDataRepository();
        }
    }
}


DataRepository* CEngineDriver::GetInputRepository()
{
    DICX* pDicX = DIX(0);

    return pDicX->IsDataRepositoryOpen() ? &pDicX->GetDataRepository() :
                                           nullptr;
}


Case& CEngineDriver::GetInputCase()
{
    return DIX(0)->GetCase();
}


int CEngineDriver::GetInputDictionaryKeyLength() const
{
    return DIP(0)->qlen[0];
}
