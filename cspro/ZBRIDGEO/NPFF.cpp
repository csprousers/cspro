#include "StdAfx.h"
#include "npff.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/BinaryGen.h>
#include <zToolsO/Serializer.h>
#include <zUtilO/AppLdr.h>
#include <zMessageO/Messages.h>

#ifdef WIN_DESKTOP
#include <zTableO/Table.h>
#endif


CNPifFile::CNPifFile(CString sFileName/* = CString()*/)
    :   PFF(sFileName),
        m_bBinaryLoad(false)
{
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CNPifFile::BuildAllObjects()
//
/////////////////////////////////////////////////////////////////////////////////
bool CNPifFile::BuildAllObjects()
{
    SetApplication(std::make_unique<Application>());

    //Open the object
    if(m_sAppFName.IsEmpty()){
        ErrorMessage::Display(FormatText("'%s' does not have the application name. Cannot open the file.", UTF8_TODO::GetUtf8(GetPifFileName()).c_str()));
        return false;
    }
    ASSERT(!m_sAppFName.IsEmpty());

    // check for binary load vs regular load
    std::string extension = Path::GetExtension(UTF8_TODO::GetUtf8(m_sAppFName));

    // use .pen file if .ent file is not there
    // this facilitates deployment since you can use same .pff file for .ent and .pen
    CString sAppFNameBin = m_sAppFName;

#ifdef WIN_DESKTOP
    if( SO::EqualsNoCase(extension, FileExtensions::EntryApplication) && !PortableFunctions::FileIsRegular(m_sAppFName) )
#endif
    {
        sAppFNameBin = UTF8_TODO::GetCString(Path::ReplaceExtension(UTF8_TODO::GetUtf8(sAppFNameBin), FileExtensions::BinaryEntryPen));

        if( PortableFunctions::FileIsRegular(sAppFNameBin) )
            extension = FileExtensions::BinaryEntryPen;
    }

    bool bOpenOK = false;

    if( SO::EqualsNoCase(extension, FileExtensions::BinaryEntryPen) || GetBinaryLoad() )
    {
        // binary load
        SetBinaryLoad(true);
        m_application->GetAppLoader()->SetBinaryFileLoad(true);

        m_application->GetAppLoader()->SetArchiveFilePath(UTF8_TODO::GetUtf8(sAppFNameBin));

        if( SO::EqualsNoCase(extension, FileExtensions::BinaryEntryPen) )
        {
            // if file extension is binary, set back to regular so that pff always writes out .ent
            PathRemoveExtension(m_sAppFName.GetBuffer());
            m_sAppFName.ReleaseBuffer();
            m_sAppFName += UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::EntryApplication));
        }

        auto serializer = std::make_shared<Serializer>();
        APP_LOAD_TODO_SetArchive(serializer);

        try
        {
            serializer->OpenInputArchive(m_application->GetAppLoader()->GetArchiveFilePath());
            *serializer & *m_application;

            m_application->SetApplicationFilePath(m_application->GetAppLoader()->GetArchiveFilePath()); // 20131202

            bOpenOK = true;
        }

        catch( const CSProException& exception ) // 20140814 display a message if one is thrown
        {
#ifndef WIN_DESKTOP
            PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog("Application Load Error", exception.what(), MB_OK);
#else
            ErrorMessage::Display(exception);
#endif
        }
        catch(...)
        {
            const std::string message = FormatText("Error reading file %s. Verify that the file exists and that it is a "
                                                   "valid CSPro .pen file and that it is located in the the correct folder.",
                                                    m_application->GetAppLoader()->GetArchiveFilePath().c_str());
#ifndef WIN_DESKTOP
            PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog("Application Load Error", message, MB_OK);
#else
            ErrorMessage::Display(message);
#endif
        }

        if( !bOpenOK )
        {
            APP_LOAD_TODO_SetArchive(nullptr);
            return false;
        }
    }
    else {
        // regular load
        m_application->GetAppLoader()->SetBinaryFileLoad(false);

        try
        {
            m_application->Open(m_sAppFName, true);
            bOpenOK = true;
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
            return false;
        }
    }

    if(!bOpenOK) {
#ifdef WIN_DESKTOP
        CString sMsg;
        sMsg.FormatMessage(IDS_APPLDFLD, m_sAppFName.GetString());
        ErrorMessage::Display(sMsg);
#endif
        return false;
    }

    //Do the external dictionaries
    if(!LoadEDicts())
        return false;

    if( BinaryGen::IsCreatingPen() )
    {
        if( !SaveEDicts() )
            return false;
    }

    //Do the form files
    if(m_application->GetEngineAppType() == EngineAppType::Tabulation ) {//TO Do // delete this on exit
#ifdef WIN_DESKTOP
        CString sTabFile = UTF8_TODO::GetCString(m_application->GetTableSpecFilePaths().front());

        BOOL bOK = TRUE;
        CSpecFile specFile(TRUE);

        bOK = specFile.Open(sTabFile,CFile::modeRead);
        if (!bOK) {
            return false;
        }

        std::vector<std::string> dictionary_file_paths = GetFileNameArrayFromSpecFile(specFile, CSPRO_DICTS);
        specFile.Close();

        if (dictionary_file_paths.empty()) {
            // &&& no dictionary name in spec file; ask for it?
            AfxMessageBox(_T("No data dictionary specified in spec file"));
            return false;
        }

        auto pTabSet = std::make_shared<CTabSet>();
        pTabSet->Open(sTabFile);
        pTabSet->SetDictFile(UTF8_TODO::GetCString(dictionary_file_paths.front()));
        CString sDictFile = pTabSet->GetDictFile();

        try
        {
            auto pDict = std::make_shared<CDataDict>();
            if(!sDictFile.IsEmpty()) {
                pDict->Open(sDictFile);
            }
            pTabSet->SetDict(pDict);
            m_application->SetTabSpec(pTabSet);

            //Set Working storage dict
            for( const std::string& dictionary_file_path : m_application->GetExternalDictionaryFilePaths() ) {
                const DictionaryDescription* const dictionary_description = m_application->GetDictionaryDescription(dictionary_file_path);
                if( dictionary_description != nullptr && dictionary_description->GetDictionaryType() == DictionaryType::Working ) {
                     auto pWDict = std::make_shared<CDataDict>();
                     if(!sDictFile.IsEmpty()) {
                         pWDict->Open(dictionary_file_path);
                         pTabSet->SetWorkDict(pWDict);
                     }
                    break;
                }
            }
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
            return false;
        }
#endif // WIN_DESKTOP
    }

    else {
        if( !LoadFormObjects() )
            return false;

        if( m_application != nullptr )
            SetFormFileNumber(*m_application);

        if( BinaryGen::IsCreatingPen() )
            SaveFormObjects();
    }

    return true;
}


bool CNPifFile::SaveEDicts()
{
    ASSERT(BinaryGen::IsCreatingPen());

    for( const std::shared_ptr<CDataDict>& dictionary : m_application->GetRuntimeExternalDictionaries() )
    {
        try // 20121109 for the portable environment
        {
            APP_LOAD_TODO_GetArchive() & *dictionary;
        }

        catch(...)
        {
            ErrorMessage::Display("There was an error writing to the binary file: " + BinaryGen::GetPenFilePath());
            return false;
        }
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CNPifFile::LoadEDicts()
//
/////////////////////////////////////////////////////////////////////////////////
bool CNPifFile::LoadEDicts()
{
    for( const std::string& dictionary_file_path : m_application->GetExternalDictionaryFilePaths() )
    {
        auto dictionary = std::make_shared<CDataDict>();
        m_application->AddRuntimeExternalDictionary(dictionary);

        if( m_application->GetAppLoader()->GetBinaryFileLoad() )
        {
            try // 20121115 for the portable environment
            {
                APP_LOAD_TODO_GetArchive() & *dictionary;
                dictionary->BuildNameList();
                dictionary->UpdatePointers();
            }

            catch( const std::exception& exception )
            {
                ErrorMessage::Display(FormatText(MGF::GetMessageText(MGF::ErrorReadingPen)->c_str(),
                                                 Path::GetFilename(m_application->GetAppLoader()->GetArchiveFilePath()).c_str(),
                                                 exception.what()));
                return false;
            }
        }

        else
        {
            try
            {
                dictionary->Open(dictionary_file_path, true);
            }

            catch( const CSProException& exception )
            {
                ErrorMessage::Display(exception);
                return false;
            }
        }

        DictionaryDescription* dictionary_description = m_application->GetDictionaryDescription(dictionary_file_path);

        if( dictionary_description == nullptr )
            dictionary_description = m_application->AddDictionaryDescription(DictionaryDescription(dictionary_file_path, DictionaryType::External));

        dictionary_description->SetDictionary(dictionary.get());
    }

    return true;
}


bool CNPifFile::SaveFormObjects()
{
    ASSERT(BinaryGen::IsCreatingPen());

    //Add the runtime dicts again
    for( const std::shared_ptr<CDEFormFile>& pFormFile : m_application->GetRuntimeFormFiles() )
    {
        try // 20121109 for the portable environment
        {
            APP_LOAD_TODO_GetArchive() & *pFormFile;
        }

        catch(...)
        {
            ErrorMessage::Display("There was an error writing to the binary file: " + BinaryGen::GetPenFilePath());
            return false;
        }
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL CNPifFile::LoadFormObjects(void)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CNPifFile::LoadFormObjects(void)
{
    //Add the runtime dicts again
    const int iCount = static_cast<int>(m_application->GetFormFilePaths().size());

    if(iCount == 0)
        return FALSE;

    for(int iIndex =0; iIndex < iCount ;iIndex++) {
        std::shared_ptr<CDEFormFile> pFormFile;

        if( iIndex == 0 || !m_application->GetAppLoader()->GetBinaryFileLoad() ) // 20121115 all the other forms will be created below
        {
            pFormFile = std::make_shared<CDEFormFile>();
            m_application->AddRuntimeFormFile(pFormFile);
        }

        else
        {
            pFormFile = m_application->GetRuntimeFormFiles()[iIndex];
        }

        const std::string& form_file_path = m_application->GetFormFilePaths()[iIndex];
        pFormFile->SetFilePath(form_file_path); // RHF Nov 13,

        bool bFileOpenError = false;
        if (m_application->GetAppLoader()->GetBinaryFileLoad())
        {
            try // 20121115 for the portable environment
            {
                if( iIndex == 0 ) // all the dictionaries are stored first
                {
                    for( int j = 0; j < iCount; j++ )
                    {
                        if( j > 0 )
                            m_application->AddRuntimeFormFile(std::make_shared<CDEFormFile>());

                        m_application->GetRuntimeFormFiles()[j]->LoadRTDicts(m_application->GetAppLoader());
                    }
                }

                APP_LOAD_TODO_GetArchive() & *pFormFile;
                bFileOpenError = false;
            }

            catch( const std::exception& exception )
            {
                ErrorMessage::Display(FormatText(MGF::GetMessageText(MGF::ErrorReadingPen)->c_str(),
                                                 m_application->GetAppLoader()->GetArchiveFilePath().c_str(),
                                                 exception.what()));
                return false;
            }
        }
        else {
            bFileOpenError = ( pFormFile->Open(form_file_path, TRUE) == false );
        }

        if( bFileOpenError )
        {
           ErrorMessage::Display("Form load failed: " + form_file_path);
           return false;
        }

        if( !m_application->GetAppLoader()->GetBinaryFileLoad() && !pFormFile->LoadRTDicts(m_application->GetAppLoader()) ) // 20121115 added first condition
            return FALSE;

        if( BinaryGen::IsCreatingPen() )
        {
            if( !pFormFile->SaveRTDicts() )
                return false;
        }

        pFormFile->UpdatePointers();

        //Set the dict desc
        DictionaryDescription* dictionary_description = m_application->GetDictionaryDescription(UTF8_TODO::GetUtf8(pFormFile->GetDictionaryFilename()), form_file_path);

        if( dictionary_description == nullptr )
        {
            dictionary_description = m_application->AddDictionaryDescription(
                DictionaryDescription(UTF8_TODO::GetUtf8(pFormFile->GetDictionaryFilename()),
                                      form_file_path,
                                      ( iIndex == 0 ) ? DictionaryType::Input : DictionaryType::External));
        }

        dictionary_description->SetDictionary(pFormFile->GetDictionary());

        if( m_application->GetAppLoader()->GetBinaryFileLoad() )
            const_cast<CDataDict*>(pFormFile->GetDictionary())->SetFilePath(UTF8_TODO::GetUtf8(pFormFile->GetDictionaryFilename()));
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CNPifFile::SetFormFileNumber(void)
//
/////////////////////////////////////////////////////////////////////////////////
void CNPifFile::SetFormFileNumber(Application& application)
{
    int iIndex = 0;

    for( const auto& pFormFile : application.GetRuntimeFormFiles() )
    {
        //For each level
        for (int iLevel =0; iLevel < pFormFile->GetNumLevels() ; iLevel++)
            SetFormFileNumber(pFormFile->GetLevel(iLevel), iIndex);

        ++iIndex;
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CNPifFile::SetFormFileNumber(CDEFormBase* pBase, int iNumber)
//
/////////////////////////////////////////////////////////////////////////////////
void CNPifFile::SetFormFileNumber(CDEFormBase* pBase, int iNumber)
{
    if(!pBase)
     return ;
    pBase->SetFormFileNumber(iNumber);

    //NDK to use typecheck use dynamic_cast since we have the type enum. we are doing it this way. -Savy
    if(pBase->GetFormItemType() == CDEFormBase::Level)
    {
        CDEGroup* pGroup = ((CDELevel*)pBase)->GetRoot();
        SetFormFileNumber(pGroup, iNumber);
    }
    else if(pBase->GetFormItemType() == CDEFormBase::Group)
    {
        CDEGroup* pGroup = (CDEGroup*)pBase;
        for(int iItem = 0; iItem < pGroup->GetNumItems() ; iItem++)
        {
            SetFormFileNumber(pGroup->GetItem(iItem), iNumber);
        }
    }
}
