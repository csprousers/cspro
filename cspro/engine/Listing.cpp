#include "StandardSystemIncludes.h"
#include "Engdrv.h"
#include "IntDrive.h"
#include <zEngineO/AllSymbols.h>
#include <zBridgeO/NPff.h>
#include <zMessageO/MessageManager.h>
#include <zListingO/ErrorLister.h>
#include <zListingO/HeaderAttribute.h>
#include <zListingO/ListerWriteFile.h>
#include <zListingO/TextWriteFile.h>

#ifdef WIN_DESKTOP
#include "Batdrv.h"
#endif


void CEngineDriver::OpenListerAndWriteFiles()
{
    ASSERT(m_lister == nullptr && m_writeFile == nullptr);

    if( !m_pPifFile->GetWriteFName().IsEmpty() )
        m_writeFile = std::make_unique<Listing::TextWriteFile>(UTF8_TODO::GetUtf8(m_pPifFile->GetWriteFName()));

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

    std::string application_type = Appl.ApplicationTypeText;
    bool cstab = false;
    bool cscalc = false;

    // listing files for entry applications are opened in append mode
    if( m_pPifFile->GetAppType() == APPTYPE::ENTRY_TYPE )
    {
        append = true;
    }

    else if( m_pPifFile->GetAppType() == APPTYPE::TAB_TYPE )
    {
        if( SO::EqualsNoCase(application_type, "CSTab") )
        {
            cstab = true;
            application_type = "Tab";
        }

        else
        {
            if( SO::EqualsNoCase(application_type, "PostCalc") )
            {
                cscalc = true;
                application_type = "Format";
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
        m_writeFile = std::make_unique<Listing::ListerWriteFile>(m_lister);


    // initialize the listing by writing out a header of various attributes
    std::vector<Listing::HeaderAttribute> header_attributes;

    header_attributes.emplace_back("Application", UTF8_TODO::GetUtf8(m_pPifFile->GetAppFName()));
    header_attributes.emplace_back("Type", application_type);

    if( cscalc )
    {
#ifdef WIN_DESKTOP
        CCalcDriver* const pCalcDriver = assert_cast<CCalcDriver*>(m_pEngineDriver);
        header_attributes.emplace_back("Input Data", UTF8_TODO::GetUtf8(pCalcDriver->GetInputTbd()->GetFileName()));
        header_attributes.emplace_back("Output", UTF8_TODO::GetUtf8(m_pPifFile->GetPrepOutputFName()));
#endif
    }

    // add the dictionaries
    else
    {
        for( const EngineDictionary* const engine_dictionary : VI_P(m_EngineArea.m_engineData->engine_dictionaries) )
        {
            if( engine_dictionary->GetSubType() == SymbolSubType::Input )
            {
                auto add_connection_strings = [&](const char* const type, const std::vector<ConnectionString>& connection_strings)
                {
                    for( const ConnectionString& connection_string : connection_strings )
                    {
                        header_attributes.emplace_back(type,
                                                       std::nullopt,
                                                       connection_string,
                                                       &engine_dictionary->GetDictionary());
                    }
                };

                add_connection_strings("Input Data", m_pPifFile->GetInputDataConnectionStrings());
                add_connection_strings("Output Data", m_pPifFile->GetOutputDataConnectionStrings());
            }

            else if( engine_dictionary->IsDictionaryObject() && engine_dictionary->GetSubType() != SymbolSubType::Work )
            {
                const ConnectionString& connection_string = m_pPifFile->GetExternalDataConnectionString(UTF8_TODO::GetCString(engine_dictionary->GetName()));

                if( connection_string.IsDefined() )
                {
                    header_attributes.emplace_back(ToString(engine_dictionary->GetSubType()),
                                                   engine_dictionary->GetName(),
                                                   connection_string,
                                                   &engine_dictionary->GetDictionary());
                }
            }
        }

        for( const DICT* const pDicT : m_engineData->dictionaries_pre80 )
        {
            if( pDicT->GetSubType() == SymbolSubType::Input )
            {
                auto add_connection_strings = [&](const char* const type, const std::vector<ConnectionString>& connection_strings)
                {
                    for( const ConnectionString& connection_string : connection_strings )
                    {
                        header_attributes.emplace_back(type,
                                                       std::nullopt,
                                                       connection_string,
                                                       pDicT->GetDataDict());
                    }
                };

                add_connection_strings("Input Data", m_pPifFile->GetInputDataConnectionStrings());
                add_connection_strings("Output Data", m_pPifFile->GetOutputDataConnectionStrings());
            }

            else if( const bool external = ( pDicT->GetSubType() == SymbolSubType::External ); external || pDicT->GetSubType() == SymbolSubType::Output )
            {
                const ConnectionString& connection_string = m_pPifFile->GetExternalDataConnectionString(UTF8_TODO::GetCString(pDicT->GetName()));

                if( connection_string.IsDefined() )
                {
                    header_attributes.emplace_back(external ? "External" : "Output",
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
        header_attributes.emplace_back("Output", UTF8_TODO::GetUtf8(m_pPifFile->GetTabOutputFName()));
    }


    auto add_non_empty_value = [&](const char* const description, const std::string& value)
    {
        if( !value.empty() )
            header_attributes.emplace_back(description, value);
    };

    add_non_empty_value("<Write>", UTF8_TODO::GetUtf8(m_pPifFile->GetWriteFName()));

    add_non_empty_value("<Impute Freq>", UTF8_TODO::GetUtf8(m_pPifFile->GetImputeFrequenciesFilename()));

    if( m_pPifFile->GetImputeStatConnectionString().IsDefined() )
        header_attributes.emplace_back("<Impute Stat>", m_pPifFile->GetImputeStatConnectionString().GetName(DataRepositoryNameType::ForListing));

    add_non_empty_value("<Paradata Log>", UTF8_TODO::GetUtf8(m_pPifFile->GetParadataFilename()));

    // add external files
    for( const LogicFile* const logic_file : m_EngineArea.m_engineData->files_global_visibility )
    {
        if( logic_file->IsUsed() )
        {
            header_attributes.emplace_back("<File>",
                                           logic_file->GetName(),
                                           UTF8_TODO::GetUtf8(m_pPifFile->LookUpUsrDatFile(UTF8_TODO::GetCString(logic_file->GetName()))));
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

    const std::function<double(int)> denominator_calculator =
        [&](const int denominator_symbol_index)
        {
            const Symbol& symbol = NPT_Ref(denominator_symbol_index);

            return symbol.IsA(SymbolType::WorkVariable) ? assert_cast<const WorkVariable&>(symbol).GetValue() :
                                                          m_pIntDriver->GetSingVarFloatValue(assert_cast<const VART*>(&symbol));
        };

    const std::vector<std::vector<MessageSummary>> message_summary_sets =
    {
        m_systemMessageManager->GenerateMessageSummaries(MessageSummary::Type::System, denominator_calculator),
        m_userMessageManager->GenerateMessageSummaries(MessageSummary::Type::UserNumbered, denominator_calculator),
        m_userMessageManager->GenerateMessageSummaries(MessageSummary::Type::UserUnnumbered, denominator_calculator)
    };

    m_lister->Finalize(*m_pPifFile, message_summary_sets);

    // close the file
    m_lister.reset();
}
