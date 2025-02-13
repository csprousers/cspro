#include "StdAfx.h"
#include "npff.h"
#include <zPlatformO/PlatformInterface.h>
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
        ErrorMessage::Display(FormatText(_T("%s does not have the application name. Cannot open the file"), this->GetPifFileName().GetString()));
        return false;
    }
    ASSERT(!m_sAppFName.IsEmpty());

    // check for binary load vs regular load
    CString sAppExt = PathFindExtension(m_sAppFName);

    // use .pen file if .ent file is not there
    // this facilitates deployment since you can use same .pff file for .ent and .pen
    CString sAppFNameBin = m_sAppFName;

#ifdef WIN_DESKTOP
    if( sAppExt.CompareNoCase(FileExtensions::WithDot::EntryApplication) == 0 && !PortableFunctions::FileIsRegular(m_sAppFName) )
#endif
    {
        sAppFNameBin = PortableFunctions::PathRemoveFileExtension<CString>(sAppFNameBin) + FileExtensions::WithDot::BinaryEntryPen;

        if( PortableFunctions::FileIsRegular(sAppFNameBin) )
            sAppExt = FileExtensions::WithDot::BinaryEntryPen;
    }

    bool bOpenOK = false;

    if( sAppExt.CompareNoCase(FileExtensions::WithDot::BinaryEntryPen) == 0 || GetBinaryLoad() )
    {
        // binary load
        SetBinaryLoad(true);
        m_application->GetAppLoader()->SetBinaryFileLoad(true);

        m_application->GetAppLoader()->SetArchiveName(CS2WS(sAppFNameBin));

        if (sAppExt.CompareNoCase(FileExtensions::WithDot::BinaryEntryPen) == 0) {
            // if file extension is binary, set back to regular so that pff always writes out .ent
            PathRemoveExtension(m_sAppFName.GetBuffer());
            m_sAppFName.ReleaseBuffer();
            m_sAppFName += FileExtensions::WithDot::EntryApplication;
        }

        auto serializer = std::make_shared<Serializer>();
        APP_LOAD_TODO_SetArchive(serializer);

        try
        {
            serializer->OpenInputArchive(m_application->GetAppLoader()->GetArchiveName());
            *serializer & *m_application;

            m_application->SetApplicationFilename(WS2CS(m_application->GetAppLoader()->GetArchiveName())); // 20131202

            bOpenOK = true;
        }

        catch( const CSProException& exception ) // 20140814 display a message if one is thrown
        {
#ifndef WIN_DESKTOP
            PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(_T("Application Load Error"), exception.GetErrorMessage(), MB_OK);
#else
            ErrorMessage::Display(exception);
#endif
        }
        catch(...)
        {
            CString sErr = FormatText(_T("Error reading file %s. Verify that the file exists and that it is a ")
                                      _T("valid CSPro .pen file and that it is located in the the correct folder."),
                                      m_application->GetAppLoader()->GetArchiveName().c_str());
#ifndef WIN_DESKTOP
            PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(_T("Application Load Error"), sErr, MB_OK);
#else
            ErrorMessage::Display(sErr);
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

#ifdef GENERATE_BINARY
    if( BinaryGen::isGeneratingBinary() )
        if(!SaveEDicts(BinaryGen::GetBinaryName()))
            return false;
#endif

    //Do the form files
    if(m_application->GetEngineAppType() == EngineAppType::Tabulation ) {//TO Do // delete this on exit
#ifdef WIN_DESKTOP
        CString sTabFile = m_application->GetTabSpecFilenames().front();

        BOOL bOK = TRUE;
        CSpecFile specFile(TRUE);

        bOK = specFile.Open(sTabFile,CFile::modeRead);
        if (!bOK) {
            return false;
        }

        std::vector<std::wstring> dictionary_filenames = GetFileNameArrayFromSpecFile(specFile, CSPRO_DICTS);
        specFile.Close();

        if (dictionary_filenames.empty()) {
            // &&& no dictionary name in spec file; ask for it?
            AfxMessageBox(_T("No data dictionary specified in spec file"));
            return false;
        }

        auto pTabSet = std::make_shared<CTabSet>();
        pTabSet->Open(sTabFile);
        pTabSet->SetDictFile(WS2CS(dictionary_filenames.front()));
        CString sDictFile = pTabSet->GetDictFile();
#ifdef USE_BINARY
        ASSERT(0);
#else
        try
        {
            auto pDict = std::make_shared<CDataDict>();
            if(!sDictFile.IsEmpty()) {
                pDict->Open(sDictFile);
            }
            pTabSet->SetDict(pDict);
            m_application->SetTabSpec(pTabSet);

            //Set Working storage dict
            for( const CString& dictionary_filename : m_application->GetExternalDictionaryFilenames() ) {
                const DictionaryDescription* dictionary_description = m_application->GetDictionaryDescription(dictionary_filename);
                if( dictionary_description != nullptr && dictionary_description->GetDictionaryType() == DictionaryType::Working ) {
                     auto pWDict = std::make_shared<CDataDict>();
                     if(!sDictFile.IsEmpty()) {
                         pWDict->Open(dictionary_filename);
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

#endif // USE_BINARY
#endif // WIN_DESKTOP
    }

    else {
        if( !LoadFormObjects() )
            return false;

        if( m_application != nullptr )
            SetFormFileNumber(*m_application);

#ifdef GENERATE_BINARY
        if( BinaryGen::isGeneratingBinary() )
            SaveFormObjects(BinaryGen::GetBinaryName());
#endif
    }

    return true;
}

#ifdef GENERATE_BINARY
bool CNPifFile::SaveEDicts(const std::wstring& archive_name)
{
    ASSERT( BinaryGen::isGeneratingBinary() );
    bool bOk = true;

    for( const auto& dictionary : m_application->GetRuntimeExternalDictionaries() )
    {
        try // 20121109 for the portable environment
        {
            APP_LOAD_TODO_GetArchive() & *dictionary;
        }

        catch(...)
        {
            ErrorMessage::Display(FormatText(_T("There was an error writing to the binary file %s"), archive_name.c_str()));
            return false;
        }
    }

    return bOk;
}
#endif

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CNPifFile::LoadEDicts()
//
/////////////////////////////////////////////////////////////////////////////////
bool CNPifFile::LoadEDicts()
{
    for( const CString& dictionary_filename : m_application->GetExternalDictionaryFilenames() )
    {
        auto dictionary = std::make_shared<CDataDict>();
        m_application->AddRuntimeExternalDictionary(dictionary);

        if( m_application->GetAppLoader()->GetBinaryFileLoad() )
        {
            // Test if USE_BINARY will use this code ...
            try // 20121115 for the portable environment
            {
                APP_LOAD_TODO_GetArchive() & *dictionary;
                dictionary->BuildNameList();
                dictionary->UpdatePointers();
            }

            catch(...)
            {
                ErrorMessage::Display(FormatText(MGF::GetMessageText(MGF::ErrorReadingPen).c_str(), m_application->GetAppLoader()->GetArchiveName().c_str()));
                return false;
            }
        }

        else
        {
            try
            {
                dictionary->Open(dictionary_filename, true);
            }

            catch( const CSProException& exception )
            {
		        ErrorMessage::Display(exception);
                return false;
            }
        }

        DictionaryDescription* dictionary_description = m_application->GetDictionaryDescription(dictionary_filename);

        if( dictionary_description == nullptr )
            dictionary_description = m_application->AddDictionaryDescription(DictionaryDescription(CS2WS(dictionary_filename), DictionaryType::External));

        dictionary_description->SetDictionary(dictionary.get());
    }

    return true;
}

#ifdef GENERATE_BINARY
bool CNPifFile::SaveFormObjects(const std::wstring& archive_name)
{
    ASSERT( BinaryGen::isGeneratingBinary() );
    if( !BinaryGen::isGeneratingBinary() )
        return true;

    bool bRet = true;

    //Add the runtime dicts again
    for( const auto& pFormFile : m_application->GetRuntimeFormFiles() )
    {
        try // 20121109 for the portable environment
        {
            APP_LOAD_TODO_GetArchive() & *pFormFile;
        }

        catch(...)
        {
            ErrorMessage::Display(FormatText(_T("There was an error writing to the binary file %s"), archive_name.c_str()));
            return false;
        }
    }

    return bRet;
}
#endif // GENERATE_BINARY

/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL CNPifFile::LoadFormObjects(void)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CNPifFile::LoadFormObjects(void)
{
    //Add the runtime dicts again
    int iCount = (int)m_application->GetFormFilenames().size();

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

        CString sFFName = m_application->GetFormFilenames()[iIndex];
        pFormFile->SetFileName( sFFName ); // RHF Nov 13,

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

            catch(...)
            {
                ErrorMessage::Display(FormatText(MGF::GetMessageText(MGF::ErrorReadingPen).c_str(), m_application->GetAppLoader()->GetArchiveName().c_str()));
                return false;
            }
        }
        else {
#ifndef USE_BINARY
           bFileOpenError = (pFormFile->Open(sFFName,TRUE) == false);
#else
           ASSERT(!_T("No non-binary file load in this build"));
           bFileOpenError = false;
#endif
        }

       if(bFileOpenError) {
#ifdef WIN_DESKTOP
            CString sMsg;
            sMsg.FormatMessage(IDS_FLDDLD, sFFName.GetString());
            AfxMessageBox(sMsg);
#endif
           return false;
       }

       if( !m_application->GetAppLoader()->GetBinaryFileLoad() && !pFormFile->LoadRTDicts(m_application->GetAppLoader())) // 20121115 added first condition
           return FALSE;

#ifdef GENERATE_BINARY
       if(BinaryGen::isGeneratingBinary())
           if(!pFormFile->SaveRTDicts(BinaryGen::GetBinaryName()))
               return false;
#endif
        pFormFile->UpdatePointers();

        //Set the dict desc
        DictionaryDescription* dictionary_description = m_application->GetDictionaryDescription(pFormFile->GetDictionaryFilename(), sFFName);

        if( dictionary_description == nullptr )
        {
            dictionary_description = m_application->AddDictionaryDescription(
                DictionaryDescription(CS2WS(pFormFile->GetDictionaryFilename()),
                                      CS2WS(sFFName),
                                      ( iIndex == 0 ) ? DictionaryType::Input : DictionaryType::External));
        }

        dictionary_description->SetDictionary(pFormFile->GetDictionary());

        if( m_application->GetAppLoader()->GetBinaryFileLoad() )
            const_cast<CDataDict*>(pFormFile->GetDictionary())->SetFullFileName(pFormFile->GetDictionaryFilename());
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
