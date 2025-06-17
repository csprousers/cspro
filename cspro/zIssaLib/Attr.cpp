//---------------------------------------------------------------------
//  Attr (was AttrLoad) - load application attributes
//---------------------------------------------------------------------
#include "StdAfx.h"
#include "CFlow.h"                                      // victor Dec 28, 99
#include "FlowCore.h"                                   // victor Jan 08, 01
#include <zUtilO/ExecutionStack.h>
#include <zBridgeO/NPff.h>
#include <zEngineO/Report.h>
#include <engine/Tables.h>
#include <engine/Engine.h>
#include <engine/Engarea.h>
#include <zCapiO/CapiQuestionManager.h>

#ifdef WIN_DESKTOP
#include <engine/Batdrv.h>
#endif


//----------------------------------------------------------------------
//  attrload: load application attributes
//
//  returns   FALSE - unable to process ATT file
//            TRUE  - can process ATT (eventual I/O errors detected).
//                    If the function is used to execute an application
//                    application must be aborted if i/o error arise.
//                    Otherwise, the application can be loaded and
//                    edited; the 'Failmsg' text should be issued as
//                    a warning to the user
//----------------------------------------------------------------------

bool CEngineDriver::attrload()
{
    Application* pApp = GetApplication();

    m_pEngineSettings->m_io_Dic.clear();
    m_pEngineSettings->m_io_Var.clear();
    m_pEngineSettings->m_io_Err = 0;
    m_pEngineSettings->m_failMessage.clear();

    // insert APP object
    if( GetSymbolTable().NameExists(Appl.GetName()) )
    {
        m_pEngineSettings->m_failMessage = FormatText("name '%s' already present", Appl.GetName().c_str());
        return false;
    }

    m_engineData->AddSymbol(m_pEngineArea->m_Appl);

    // application type                 // TODO: add more types to CsPro???
    std::optional<std::tuple<ModuleType, const char*>> application_type;

    if( pApp->GetEngineAppType() == EngineAppType::Entry )
    {
        application_type.emplace(ModuleType::Entry, "ENTRY");
    }

    else if( pApp->GetEngineAppType() == EngineAppType::Batch )
    {
        application_type.emplace(ModuleType::Batch, "BATCH");

        // RHF INIC Jan 31, 2003
        if( Issamod == ModuleType::Batch )
        {
#ifdef WIN_DESKTOP
            CBatchDriverBase* const pBatchDriverBase = assert_cast<CBatchDriverBase*>(this);

            if( pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC )
            {
                std::get<1>(*application_type) = "POSTCALC";
            }

            else if( pBatchDriverBase->GetBatchMode() == CRUNAPL_CSTAB )
            {
                std::get<1>(*application_type) = "CSTAB";
            }
#endif
        }
        // RHF END Jan 31, 2003
    }

    if( !application_type.has_value() )
    {
        m_pEngineSettings->m_io_Err = 1;
        m_pEngineSettings->m_failMessage = "invalid application type";
        return false;
    }

    Appl.ApplicationType = std::get<0>(*application_type);
    Appl.ApplicationTypeText = std::get<1>(*application_type);

    // inserting Appl' children (Flows/Dicts/Flow Forms)// victor Dec 27, 99
    // into symbol table -- returns if any error        // victor Dec 27, 99
    if( !m_pEngineArea->MakeApplChildren() )
    {
        m_pEngineDriver->issaerror(MessageType::Error, 10050);
        return false;
    }

    return true;
}


bool CEngineArea::MakeApplChildren()
{
    // creating Application' children Dicts & Flows
    Application* pApp = m_pEngineDriver->GetApplication();
    bool bDesigner = ( Issamod == ModuleType::Designer );
    int iDicSeqNo = 0; // to help set the dic' role

    // preparing Flows and External dictionaries to be inserted:
    // a.- updating pointers of every Flow' dictionaries
    for( const auto& pFormFile : pApp->GetRuntimeFormFiles() )
    {
        // ... makes sure the correct info is installed into CDEFormFile
        pFormFile->UpdatePointers();

        // for each dictionary...makes sure the correct info is installed into CDEDataDict
        const_cast<CDataDict*>(pFormFile->GetDictionary())->UpdatePointers(); // DD_STD_REFACTOR_TODO should not be necessary when finished
    }

    // b.- updating pointers of External dictionaries
    for( const auto& dictionary : pApp->GetRuntimeExternalDictionaries() )
    {
        // ... makes sure the correct info is installed into CDEDataDict
        dictionary->UpdatePointers();
    }


    // (1) inserting Application' Flows (coming from FormFiles) and its Dics/Forms
    for( int iNumFlow = 0; iNumFlow < (int)pApp->GetRuntimeFormFiles().size(); iNumFlow++ )
    {
        CDEFormFile* pFormFile = pApp->GetRuntimeFormFiles()[iNumFlow].get();

        // ... insert the Flow name corresponding to this FormFile
        if( GetSymbolTable().NameExists(UTF8_TODO::GetUtf8(pFormFile->GetName())) )
        {
            m_pEngineDriver->issaerror(MessageType::Error, 10051, UTF8_TODO::GetUtf8(pFormFile->GetName()).c_str());
            return false;
        }

        auto pFlow = std::make_shared<FLOW>(UTF8_TODO::GetUtf8(pFormFile->GetName()), m_pEngineArea);
        int iSymFlow = m_engineData->AddSymbol(pFlow);

        // set links to engine into flow-core object              // victor Jan 08, 00
        pFlow->GetFlowCore()->SetEngineDriver( m_pEngineDriver ); // victor Jan 08, 00

        // set the attached FormFile descriptor
        pFlow->SetFormFile( pFormFile );

        // add to list of Flows of the APPL
        Appl.AddFlow( pFlow.get() );

        // set the Flow subtype to Primary or Secondary
        if( iNumFlow < 1 )
            pFlow->SetSubType( SymbolSubType::Primary );
        else
            pFlow->SetSubType( SymbolSubType::Secondary );


        // (a) for the dictionary of this FormFile...
        const CDataDict* pDataDict = pFormFile->GetDictionary();
        DictionaryType dictionary_type = pApp->GetDictionaryType(*pDataDict);

        // ... insert the Dictionary into symbol table
        if( iNumFlow == 0 && dictionary_type != DictionaryType::Input  ||
            iNumFlow >= 1 && ( dictionary_type != DictionaryType::External && dictionary_type != DictionaryType::Output && dictionary_type != DictionaryType::Working ) )// RHF Nov 07, 2002 Add dictionary_type != DictionaryType::Working  Allows WORKING with FLOW for using the FORMS in WRITEFORM
        {
            m_pEngineDriver->issaerror(bDesigner ? MessageType::Error : MessageType::Abort, 10059, ToString(dictionary_type),
                                       pDataDict->GetName().c_str(), "in Primary Flow" );

            if( !bDesigner )
                return false;
        }

        // make sure that there isn't a dictionary with the same name already inserted
        if( GetSymbolTable().NameExists(pDataDict->GetName()) )
        {
            m_pEngineDriver->issaerror(MessageType::Error, 10055, pFlow->GetName().c_str(), pDataDict->GetName().c_str());
            return false;
        }

        auto pDicT = std::make_shared<DICT>(pDataDict->GetName(), m_pEngineDriver); // ENGINECR_TODO need to handle loading non-external dictionaries
        int iSymDic = m_engineData->AddSymbol(pDicT);

        for( const std::string& alias : pDataDict->GetAliases() )
            GetSymbolTable().AddAlias(alias, *pDicT);

        // add to list of Dictionaries of this FLOW
        pFlow->AddDic( iSymDic );

        // pass symbol to CDEDataDict
        const_cast<CDataDict*>(pDataDict)->SetSymbol( iSymDic );         // RHF 2/8/99

        // install CDEDataDict into DICT entry
        pDicT->SetDataDict( pFormFile->GetSharedDictionary() );

        // set the Dictionary subtype
        if( pFlow->GetSubType() == SymbolSubType::Primary )
        {
            if( iDicSeqNo < 1 )
                pDicT->SetSubType( SymbolSubType::Input );
            else
                pDicT->SetSubType( SymbolSubType::Output );
        }
        // for Secondary flows, always External
        else
            pDicT->SetSubType( SymbolSubType::External );

        iDicSeqNo++;
        // ...Dict <end>


        // (b) for each Form of this FormFile...
        for( int iNumForm = 0; iNumForm < pFormFile->GetNumForms(); iNumForm++ )
        {
            CDEForm* pForm = pFormFile->GetForm(iNumForm);

            // ... insert the Form into symbol table
            auto pFormT = std::make_shared<FORM>(UTF8_TODO::GetUtf8(pForm->GetName()), pForm);
            int iSymForm = m_engineData->AddSymbol(pFormT);

            // add to list of Forms of this FLOW
            pFlow->AddForm(iSymForm);

            // pass symbol to CDEForm
            pForm->SetSymbol(iSymForm);

            // set the Form subtype to Primary or Secondary
            pFormT->SetSubType(NPT(iSymFlow)->GetSubType());
            // attach this Form <end>
        } // ...for each Form <end>

    } // create CFlow <end>


    // (2) inserting Application' External dictionaries and Working dictionaries
    for( const auto& dictionary : pApp->GetRuntimeExternalDictionaries() )
    {
        CDataDict* pDataDict = dictionary.get();
        DictionaryType dictionary_type = pApp->GetDictionaryType(*pDataDict);

        if( dictionary_type == DictionaryType::Unknown )
            dictionary_type = DictionaryType::External; // TODO Always GetDictionaryType return INVLDTYPE when used from CsPro

        if( dictionary_type == DictionaryType::Output )
            m_pEngineDriver->SetHasOutputDict(true);

        // ... insert the Dictionary into symbol table
        if( dictionary_type != DictionaryType::Working && dictionary_type != DictionaryType::External && dictionary_type != DictionaryType::Output )
        {
            m_pEngineDriver->issaerror(bDesigner ? MessageType::Error : MessageType::Abort, 10059,
                                       ToString(dictionary_type), pDataDict->GetName().c_str(), "");

            if( !bDesigner )
                return false;
        }

        // make sure that there isn't a dictionary with the same name already inserted
        if( GetSymbolTable().NameExists(pDataDict->GetName()) )
        {
            m_pEngineDriver->issaerror(MessageType::Error, 10057, ToString(dictionary_type), pDataDict->GetName().c_str());
            return false;
        }

        auto pDicT = std::make_shared<DICT>(pDataDict->GetName(), m_pEngineDriver);
        int iSymDic = m_engineData->AddSymbol(pDicT);

        for( const std::string& alias : pDataDict->GetAliases() )
            GetSymbolTable().AddAlias(alias, *pDicT);

        // create a virtual CFlow for this External (hidden name given)
        auto pFlow = std::make_shared<FLOW>("__EFlow_" + pDataDict->GetName(), m_pEngineArea);
        m_engineData->AddSymbol(pFlow);

        // create CFlow <begin>

        // set a null descriptor for attached FormFile
        pFlow->SetFormFile(NULL);

        // add to list of Flows of the APPL
        Appl.AddFlow( pFlow.get() );

        // set the Flow subtype to External
        pFlow->SetSubType( SymbolSubType::External );

        // attach this Dict <begin>

        // add to list of Dictionaries of this hidden FLOW
        pFlow->AddDic( iSymDic );

        // install CDEDataDict into DICT entry
        pDicT->SetDataDict( dictionary );

        // pass symbol to CDEDataDict
        pDataDict->SetSymbol(pDicT->GetSymbolIndex());

        // set the Dictionary subtype
        pDicT->SetSubType(( dictionary_type == DictionaryType::Working )  ? SymbolSubType::Work :
                          ( dictionary_type == DictionaryType::External ) ? SymbolSubType::External :
                                                                            SymbolSubType::Output);

        // attach this Dict <end>
        iDicSeqNo++;

    } // ...for each Dict <end>


    // (3) inserting WorkDict, which is no longer used, but .pen files still have references to it
    auto work_dict = std::make_unique<DICT>("_WORKDICT", m_pEngineDriver);
    work_dict->SetSubType(SymbolSubType::Work);
    m_engineData->AddSymbol(std::move(work_dict));


    // (4) insert reports
    for( const ReportFile& report_file : pApp->GetReportFiles() )
    {
        // make sure that the report name is unique
        if( GetSymbolTable().NameExists(report_file.GetName()) )
        {
            m_pEngineDriver->issaerror(MessageType::Error, 10060, report_file.GetName().c_str());
            return false;
        }

        m_engineData->AddSymbol(std::make_unique<Report>(report_file));
    }


    return true;
}


//////////////////////////////////////////////////////////////////////////////////

void CEngineDriver::SetPifFile(CNPifFile* pPifFile)
{
    ASSERT(pPifFile != nullptr && m_pApplication == pPifFile->GetApplication());
    m_pPifFile = pPifFile;
    m_engineData->pff = m_pPifFile;

    ASSERT(m_executionStackEntry == nullptr && ExecutionStack::GetEntries().empty());
    m_executionStackEntry = std::make_unique<ExecutionStackEntry>(ExecutionStack::AddEntry(m_pPifFile));
}


//////////////////////////////////////////////////////////////////////////////////

void CEngineDriver::InitAppName()
{
    // InitAppName: get the "LevelZero" app-name either form 1st FormFile, or from the application' file-name
    ASSERT(m_pApplication != nullptr);

    // try to get the 1st FormFile (or Flow) name
    const std::vector<std::shared_ptr<CDEFormFile>>& form_files = m_pApplication->GetRuntimeFormFiles();

    std::string level_zero_name = !form_files.empty() ? UTF8_TODO::GetUtf8(m_pApplication->GetRuntimeFormFiles().front()->GetName()) :
                                                        std::string();

    // if no name yet, get the application' file-name
    if( level_zero_name.empty() )
    {
        ASSERT(false);
        level_zero_name = CIMSAString::MakeName(Path::GetFilenameWithoutExtension(m_pApplication->GetApplicationFilePath()));
    }

    // pass the Level-zero name to the settings
    m_pEngineSettings->SetLevelZeroName(std::move(level_zero_name));
}
