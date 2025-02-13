//---------------------------------------------------------------------
//  Attr (was AttrLoad) - load application attributes
//---------------------------------------------------------------------
#include "StdAfx.h"
#include "CFlow.h"                                      // victor Dec 28, 99
#include "FlowCore.h"                                   // victor Jan 08, 01
#include <zUtilO/ExecutionStack.h>
#include <ZBRIDGEO/npff.h>
#include <zEngineO/Report.h>
#include <engine/Tables.h>
#include <engine/Engine.h>
#include <engine/Engarea.h>
#include <engine/Batdrv.h>
#include <zCapiO/CapiQuestionManager.h>


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

    io_Dic.Empty();
    io_Var.Empty();
    io_Err = 0;
    Failmsg.Empty();

    // insert APP object
    if( GetSymbolTable().NameExists(Appl.GetName()) )
    {
        Failmsg.Format(_T("name '%s' already present"), Appl.GetName().c_str());
        return false;
    }

    m_engineData->AddSymbol(m_pEngineArea->m_Appl);

    // application type                 // TODO: add more types to CsPro???
    const TCHAR* pWord = _T("none");
    ModuleType eApplType = ModuleType::None;

    if( pApp->GetEngineAppType() == EngineAppType::Entry ) {
        pWord = _T("ENTRY");
        eApplType = ModuleType::Entry;
    }
    else if( pApp->GetEngineAppType() == EngineAppType::Batch ) {
#ifdef USE_BINARY
        ASSERT(0);
#else
        pWord = _T("BATCH");
        eApplType = ModuleType::Batch;

        // RHF INIC Jan 31, 2003
        if( Issamod == ModuleType::Batch ) {
            CBatchDriverBase*   pBatchDriverBase=(CBatchDriverBase*) this;

            bool    bCsCalc = (pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC );
            bool    bCsTab = (pBatchDriverBase->GetBatchMode() == CRUNAPL_CSTAB );

            if( bCsCalc )
                pWord = _T("POSTCALC");
            else if( bCsTab )
                pWord = _T("CSTAB");
        }
        // RHF END Jan 31, 2003
#endif
    }

    if( eApplType == ModuleType::None ) {
        io_Err = 1;                 // invalid application type
        Failmsg = _T("invalid application type");
        return FALSE;
    }

    Appl.ApplicationType = eApplType;
    Appl.ApplicationTypeText = pWord;

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
        if( GetSymbolTable().NameExists(pFormFile->GetName()) )
        {
            m_pEngineDriver->issaerror(MessageType::Error, 10051, pFormFile->GetName().GetString());
            return false;
        }

        auto pFlow = std::make_shared<FLOW>(CS2WS(pFormFile->GetName()), m_pEngineArea);
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
                                       pDataDict->GetName().GetString(), _T("in Primary Flow") );

            if( !bDesigner )
                return false;
        }

        // make sure that there isn't a dictionary with the same name already inserted
        if( GetSymbolTable().NameExists(pDataDict->GetName()) )
        {
            m_pEngineDriver->issaerror(MessageType::Error, 10055, pFlow->GetName().c_str(), pDataDict->GetName().GetString());
            return false;
        }

        auto pDicT = std::make_shared<DICT>(CS2WS(pDataDict->GetName()), m_pEngineDriver); // ENGINECR_TODO need to handle loading non-external dictionaries
        int iSymDic = m_engineData->AddSymbol(pDicT);

        for( const CString& alias : pDataDict->GetAliases() )
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
            auto pFormT = std::make_shared<FORM>(CS2WS(pForm->GetName()), pForm);
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
            m_pEngineDriver->issaerror( bDesigner ? MessageType::Error : MessageType::Abort, 10059, ToString(dictionary_type), pDataDict->GetName().GetString(), _T("") );

            if( !bDesigner )
                return false;
        }

        // make sure that there isn't a dictionary with the same name already inserted
        if( GetSymbolTable().NameExists(pDataDict->GetName()) )
        {
            m_pEngineDriver->issaerror(MessageType::Error, 10057, ToString(dictionary_type), pDataDict->GetName().GetString());
            return false;
        }

        auto pDicT = std::make_shared<DICT>(CS2WS(pDataDict->GetName()), m_pEngineDriver);
        int iSymDic = m_engineData->AddSymbol(pDicT);

        for( const CString& alias : pDataDict->GetAliases() )
            GetSymbolTable().AddAlias(alias, *pDicT);

        // create a virtual CFlow for this External (hidden name given)
        CString csExternalFlowName;
        csExternalFlowName.Format( _T("__EFlow_%s"), pDataDict->GetName().GetString() );

        auto pFlow = std::make_shared<FLOW>(CS2WS(csExternalFlowName), m_pEngineArea);
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


    // (3) inserting WorkDict
    auto work_dict = std::make_shared<DICT>(_T("_WORKDICT"), m_pEngineDriver);
    Workdict = work_dict.get();
    work_dict->SetSubType(SymbolSubType::Work);
    m_engineData->AddSymbol(work_dict);


    // (4) insert reports
    for( const NamedTextSource& report_named_text_source : VI_V(pApp->GetReportNamedTextSources()) )
    {
        // make sure that the report name is unique
        if( GetSymbolTable().NameExists(report_named_text_source.name) )
        {
            m_pEngineDriver->issaerror(MessageType::Error, 10060, report_named_text_source.name.c_str());
            return false;
        }

        auto report = std::make_unique<Report>(report_named_text_source.name, report_named_text_source.text_source->GetFilename());
        m_engineData->AddSymbol(std::move(report));
    }


    return true;
}


//////////////////////////////////////////////////////////////////////////////////

void CEngineDriver::SetPifFile(CNPifFile* pPifFile)
{
    ASSERT(pPifFile != nullptr && m_pApplication == pPifFile->GetApplication());
    m_pPifFile = pPifFile;

    ASSERT(m_executionStackEntry == nullptr && ExecutionStack::GetEntries().empty());
    m_executionStackEntry = std::make_unique<ExecutionStackEntry>(ExecutionStack::AddEntry(m_pPifFile));
}


//////////////////////////////////////////////////////////////////////////////////

void CEngineDriver::InitAppName()
{
    // InitAppName: get the "LevelZero" app-name either form 1st FormFile, or from the application' file-name
    Application* application = GetApplication();

    const CodeFile* logic_main_code_file = application->GetLogicMainCodeFile();

    if( logic_main_code_file != nullptr )
        m_csAppFullName = WS2CS(logic_main_code_file->GetFilename());

    CString csNodeName = PortableFunctions::PathGetFilenameWithoutExtension<CString>(m_csAppFullName);
    CString csLevelZeroName;

    // try to get the 1st FormFile (or Flow) name
    if( !application->GetRuntimeFormFiles().empty() )
        csLevelZeroName = application->GetRuntimeFormFiles().front()->GetName();

    // if no name yet, get the application' file-name
    if( csLevelZeroName.IsEmpty() )
        csLevelZeroName = csNodeName;

    // pass the Level-zero name to the settings
    m_pEngineSettings->SetLevelZeroName(csLevelZeroName);

    // Calcute ...
    ApplName = csNodeName;
    ApplName.MakeUpper();
}
