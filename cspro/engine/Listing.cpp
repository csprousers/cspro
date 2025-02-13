#include "StandardSystemIncludes.h"
#include "Engdrv.h"
#include "Batdrv.h"
#include "IntDrive.h"
#include <zEngineO/AllSymbols.h>
#include <ZBRIDGEO/npff.h>
#include <zMessageO/MessageManager.h>
#include <zListingO/HeaderAttribute.h>
#include <zListingO/ListerWriteFile.h>
#include <zListingO/TextWriteFile.h>


void CEngineDriver::OpenListerAndWriteFiles()
{
    ASSERT(m_lister == nullptr && m_writeFile == nullptr);

    if( !m_pPifFile->GetWriteFName().IsEmpty() )
        m_writeFile = std::make_unique<Listing::TextWriteFile>(CS2WS(m_pPifFile->GetWriteFName()));

    StartLister();
}


void CEngineDriver::CloseListerAndWriteFiles()
{
    m_writeFile.reset();
    m_compilerErrorLister.reset();
    StopLister();
}


void CEngineDriver::StartLister()
{
    ASSERT(m_lister == nullptr);

    bool append = false;

    std::wstring application_type = CS2WS(Appl.ApplicationTypeText);
    bool cstab = false;
    bool cscalc = false;

    // listing files for entry applications are opened in append mode
    if( m_pPifFile->GetAppType() == APPTYPE::ENTRY_TYPE )
    {
        append = true;
    }

    else if( m_pPifFile->GetAppType() == APPTYPE::TAB_TYPE )
    {
        if( SO::EqualsNoCase(application_type, _T("CSTab")) )
        {
            cstab = true;
            application_type = _T("Tab");
        }

        else
        {
            if( SO::EqualsNoCase(application_type, _T("PostCalc")) )
            {
                cscalc = true;
                application_type = _T("Format");
            }

            // append mode will also be used if not on the first operation of a multi-step tabulation
            if( m_pPifFile->GetTabProcess() == PROCESS::ALL_STUFF )
                append = true;
        }
    }


    // create the lister
    if( UseNewDriver() )
    {
        const EngineDictionary& input_engine_dictionary = *m_EngineArea.m_engineData->engine_dictionaries.front();
        m_lister = Listing::Lister::Create(m_processSummary, *m_pPifFile, append, input_engine_dictionary.GetSharedCaseAccess());
    }

    else
    {
        const DICT* pMainDicT = DIP(0);
        m_lister = Listing::Lister::Create(m_processSummary, *m_pPifFile, append, pMainDicT->GetSharedCaseAccess());
    }

    // if no write filename is defined, hookup the write function with the lister
    if( m_pPifFile->GetWriteFName().IsEmpty() )
        m_writeFile = std::make_unique<Listing::ListerWriteFile<std::shared_ptr<Listing::Lister>>>(m_lister);


    // initialize the listing by writing out a header of various attributes
    std::vector<Listing::HeaderAttribute> header_attributes;

    header_attributes.emplace_back(_T("Application"), CS2WS(m_pPifFile->GetAppFName()));
    header_attributes.emplace_back(_T("Type"), application_type);

    if( cscalc )
    {
#if defined(WIN_DESKTOP) && !defined(USE_BINARY)
        CCalcDriver* pCalcDriver = (CCalcDriver*)m_pEngineDriver;
        header_attributes.emplace_back(_T("Input Data"), CS2WS(pCalcDriver->GetInputTbd()->GetFileName()));
        header_attributes.emplace_back(_T("Output"), CS2WS(m_pPifFile->GetPrepOutputFName()));
#endif
    }

    // add the dictionaries
    else
    {
        for( const EngineDictionary* engine_dictionary : VI_P(m_EngineArea.m_engineData->engine_dictionaries) )
        {
            if( engine_dictionary->GetSubType() == SymbolSubType::Input )
            {
                auto add_connection_strings = [&](const TCHAR* type, const std::vector<ConnectionString>& connection_strings)
                {
                    for( const ConnectionString& connection_string : connection_strings )
                    {
                        header_attributes.emplace_back(type,
                                                       std::nullopt,
                                                       connection_string,
                                                       &engine_dictionary->GetDictionary());
                    }
                };

                add_connection_strings(_T("Input Data"), m_pPifFile->GetInputDataConnectionStrings());
                add_connection_strings(_T("Output Data"), m_pPifFile->GetOutputDataConnectionStrings());
            }

            else if( engine_dictionary->IsDictionaryObject() && engine_dictionary->GetSubType() != SymbolSubType::Work )
            {
                const ConnectionString& connection_string = m_pPifFile->GetExternalDataConnectionString(WS2CS(engine_dictionary->GetName()));

                if( connection_string.IsDefined() )
                {
                    header_attributes.emplace_back(ToString(engine_dictionary->GetSubType()),
                                                   engine_dictionary->GetName(),
                                                   connection_string,
                                                   &engine_dictionary->GetDictionary());
                }
            }
        }

        for( const DICT* pDicT : m_engineData->dictionaries_pre80 )
        {
            if( pDicT->GetSubType() == SymbolSubType::Input )
            {
                auto add_connection_strings = [&](const TCHAR* type, const std::vector<ConnectionString>& connection_strings)
                {
                    for( const ConnectionString& connection_string : connection_strings )
                    {
                        header_attributes.emplace_back(type,
                                                       std::nullopt,
                                                       connection_string,
                                                       pDicT->GetDataDict());
                    }
                };

                add_connection_strings(_T("Input Data"), m_pPifFile->GetInputDataConnectionStrings());
                add_connection_strings(_T("Output Data"), m_pPifFile->GetOutputDataConnectionStrings());
            }

            else if( bool external = ( pDicT->GetSubType() == SymbolSubType::External ); external || pDicT->GetSubType() == SymbolSubType::Output )
            {
                const ConnectionString& connection_string = m_pPifFile->GetExternalDataConnectionString(WS2CS(pDicT->GetName()));

                if( connection_string.IsDefined() )
                {
                    header_attributes.emplace_back(external ? _T("External") : _T("Output"),
                                                   pDicT->GetName(),
                                                   connection_string,
                                                   pDicT->GetDataDict());
                }
            }
        }
    }

    if( cstab )
    {
        m_lister->SetUpdateProcessSummaryWithMessageNumbers(true);
        header_attributes.emplace_back(_T("Output"), CS2WS(m_pPifFile->GetTabOutputFName()));
    }


    auto add_non_empty_value = [&](const TCHAR* description, const std::wstring& value)
    {
        if( !value.empty() )
            header_attributes.emplace_back(description, value);
    };

    add_non_empty_value(_T("<Write>"), CS2WS(m_pPifFile->GetWriteFName()));

    add_non_empty_value(_T("<Impute Freq>"), CS2WS(m_pPifFile->GetImputeFrequenciesFilename()));

    if( m_pPifFile->GetImputeStatConnectionString().IsDefined() )
        header_attributes.emplace_back(_T("<Impute Stat>"), m_pPifFile->GetImputeStatConnectionString().GetName(DataRepositoryNameType::ForListing));

    add_non_empty_value(_T("<Paradata Log>"), CS2WS(m_pPifFile->GetParadataFilename()));

    // add external files
    for( const LogicFile* logic_file : m_EngineArea.m_engineData->files_global_visibility )
    {
        if( logic_file->IsUsed() )
        {
            header_attributes.emplace_back(_T("<File>"),
                                           logic_file->GetName(),
                                           CS2WS(m_pPifFile->LookUpUsrDatFile(WS2CS(logic_file->GetName()))));
        }
    }

    // set up the listing type
    if( cscalc )
    {
        m_processSummary->SetAttributesType(ProcessSummary::AttributesType::Slices);
        m_processSummary->SetNumberLevels(0);
    }

    else
    {
        m_processSummary->SetAttributesType(ProcessSummary::AttributesType::Records);

        if( UseNewDriver() )
        {
            const EngineDictionary& input_engine_dictionary = *m_EngineArea.m_engineData->engine_dictionaries.front();
            m_processSummary->SetNumberLevels(input_engine_dictionary.GetDictionary().GetNumLevels());
        }

        else
        {
            m_processSummary->SetNumberLevels(DIP(0)->maxlevel);
        }
    }

    m_lister->WriteHeader(header_attributes);
}


void CEngineDriver::StopLister()
{
    if( m_lister == nullptr )
        return;

    // if the write file is associated with the lister, close it
    if( m_pPifFile->GetWriteFName().IsEmpty() )
        m_writeFile.reset();

    // write out the listing summary, which will include message summaries

    std::function<double(int)> denominator_calculator = [&](int denominator_symbol_index) -> double
    {
        Symbol* symbol = NPT(denominator_symbol_index);

        if( symbol->IsA(SymbolType::WorkVariable) )
        {
            return assert_cast<WorkVariable*>(symbol)->GetValue();
        }

        else
        {
            return m_pIntDriver->GetSingVarFloatValue(assert_cast<VART*>(symbol));
        }
    };

    std::vector<std::vector<MessageSummary>> message_summary_sets =
    {
        m_systemMessageManager->GenerateMessageSummaries(MessageSummary::Type::System, denominator_calculator),
        m_userMessageManager->GenerateMessageSummaries(MessageSummary::Type::UserNumbered, denominator_calculator),
        m_userMessageManager->GenerateMessageSummaries(MessageSummary::Type::UserUnnumbered, denominator_calculator)
    };

    m_lister->Finalize(*m_pPifFile, message_summary_sets);

    // close the file
    m_lister.reset();
}
