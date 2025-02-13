#include "StdAfx.h"
#include "PifDlgBuilder.h"
#include <zUtilO/StdioFileUnicode.h>
#include <zUtilO/Specfile.h>
#include <zUtilO/Interapp.h>
#include <zJson/JsonStream.h>


PifDlgBuilder::PifDlgBuilder(CNPifFile * pPffFile)
    :   m_pPifFile(pPffFile)
{
}

PifDlgBuilder::~PifDlgBuilder()
{
    DeletePifInfos();
}

bool PifDlgBuilder::BuildPifInfo(PROCESS eCurrentProcess, bool bNoOutPutFiles4ALLMode)
{
    //check the mode .
    //check if area file name are reqd
    //check if the dialog is reqd or a simple dialog will Do if All is given
    //External files are required .
    // if all is given you can do this first and check if pifdlg is required .
    //else you can call simple version of the file dialog and save it .
    //write file

    //Get the application file
    DeletePifInfos();
    CNPifFile oldPFFile(m_pPifFile->GetPifFileName());
    bool bUseStateInfo = false;
    if( PortableFunctions::FileExists(m_pPifFile->GetPifFileName()) ) {
        if (oldPFFile.LoadPifFile()) {
            PROCESS eOldProcess = oldPFFile.GetTabProcess();
            if (eOldProcess == eCurrentProcess) {
                bUseStateInfo = true;
            }
        }
    }
    Application* pApp = m_pPifFile->GetApplication();
    CIMSAString sXTSFile = pApp->GetTabSpec()->GetSpecFile();
    bool bHasArea = pApp->GetTabSpec()->GetConsolidate()->GetNumAreas() > 0;
    const CDataDict* pDict = pApp->GetTabSpec()->GetDict();
    //Get the dictionary file . This is the Inputdata
    if (pDict && (eCurrentProcess == ALL_STUFF || eCurrentProcess == CS_TAB)) {
        PIFINFO* pPifInfo = new PIFINFO;
        pPifInfo->eType = PIFDICT;
        pPifInfo->sUName = pDict->GetName();
        pPifInfo->sDisplay = INPUTDATA;
        pPifInfo->uOptions = PIF_FILE_MUST_EXIST | PIF_MULTIPLE_FILES | PIF_READ_ONLY;
        pPifInfo->dictionary_filename = pDict->GetFullFileName();
        if( bUseStateInfo )
            pPifInfo->SetConnectionStrings(oldPFFile.GetInputDataConnectionStringsSerializable());
        m_arrPifInfo.Add(pPifInfo);

        //Now do each of the external dicts
        for( const CString& dictionary_filename : pApp->GetExternalDictionaryFilenames() )
        {
            const DictionaryDescription* dictionary_description = pApp->GetDictionaryDescription(dictionary_filename);

            if( dictionary_description != nullptr && dictionary_description->GetDictionaryType() == DictionaryType::Working )
                continue;

            CString dictionary_name;

            try
            {
                dictionary_name = JsonStream::GetValueFromSpecFile<CString, CDataDict>(JK::name, dictionary_filename);
            }

            catch( const CSProException& )
            {
                return false;
            }

            PIFINFO* pPifInfo = new PIFINFO;
            pPifInfo->eType = PIFDICT;
            pPifInfo->sUName = dictionary_name;
            pPifInfo->sDisplay = _T("External File ");
            pPifInfo->sDisplay += _T("(") + dictionary_name + _T(")");
            pPifInfo->uOptions = 0;
            pPifInfo->dictionary_filename = dictionary_filename;

            if( bUseStateInfo )
                pPifInfo->SetConnectionString(oldPFFile.GetExternalDataConnectionString(dictionary_name));

            //Add pff Info for the dict
            m_arrPifInfo.Add(pPifInfo);
        }

        //Now check for write file
        if (pApp->GetHasWriteStatements()) { //only for tab and All
            PIFINFO* pPifInfo = new PIFINFO;
            pPifInfo->eType = FILE_NONE;
            pPifInfo->sUName = WRITEFILE;
            pPifInfo->sFileName = oldPFFile.GetWriteFName();
            pPifInfo->sDisplay = WRITEFILE;
            pPifInfo->uOptions = PIF_ALLOW_BLANK;
            m_arrPifInfo.Add(pPifInfo);
        }

        //Now add the OutputTBD
        if (eCurrentProcess == CS_TAB) { //only for tab
            PIFINFO* pPifInfo = new PIFINFO;
            pPifInfo->eType = FILE_NONE;
            pPifInfo->sUName = OUTPUTTBD;
            pPifInfo->sFileName = oldPFFile.GetTabOutputFName(); //Get the output TBD from the pff file
            pPifInfo->sDisplay = OUTPUTTBD;
            pPifInfo->sDefaultFileExtension = FileExtensions::BinaryTable::Tab;
            m_arrPifInfo.Add(pPifInfo);
            pPifInfo->uOptions = 0;
        }
        if (eCurrentProcess == ALL_STUFF) {
            PIFINFO* pPifInfo = new PIFINFO;

            pPifInfo->eType = FILE_NONE;
            if (true) {//Ask TBW always Bruce et al revision
                pPifInfo->sUName = OUTPUTTBW;
                if (bUseStateInfo) {
                    pPifInfo->sFileName = oldPFFile.GetPrepOutputFName();
                }
                else {
                    pPifInfo->sFileName = m_pPifFile->GetAppFName() + FileExtensions::WithDot::Table;
                }
                pPifInfo->sDisplay = OUTPUTTBW;
                pPifInfo->uOptions = 0;
                pPifInfo->sDefaultFileExtension = FileExtensions::Table;
                m_arrPifInfo.Add(pPifInfo);
            }
            if (bHasArea) { //If it has area ask for areanames file
                PIFINFO* pPifInfo = new PIFINFO;
                pPifInfo->eType = FILE_NONE;
                pPifInfo->sUName = AREANAMES;
                if( bUseStateInfo )
                    pPifInfo->sFileName = oldPFFile.GetAreaFName();
                pPifInfo->sDisplay = AREANAMES;
                pPifInfo->uOptions = PIF_FILE_MUST_EXIST | PIF_READ_ONLY | PIF_ALLOW_BLANK;
                m_arrPifInfo.Add(pPifInfo);
            }
        }
        //Now do the listing file
        if (!bNoOutPutFiles4ALLMode) {
            //Now do the listing file
            PIFINFO* pPifInfo = new PIFINFO;
            if (bUseStateInfo && !oldPFFile.GetListingFName().IsEmpty()) {
                pPifInfo->sFileName = oldPFFile.GetListingFName();
            }
            else {
                pPifInfo->sFileName = m_pPifFile->GetAppFName();
                pPifInfo->sFileName = MakeOutputFileForProcess(eCurrentProcess, pPifInfo->sFileName, _T(""), FileExtensions::Listing);
            }
            //do listing
            pPifInfo->eType = FILE_NONE;
            pPifInfo->sUName = LISTFILE;
            //pPifInfo->sFileName  = oldPFFile.GetListingFName();
            pPifInfo->sDisplay = LISTFILE;
            pPifInfo->uOptions = 0;
            m_arrPifInfo.Add(pPifInfo);
        }
    }
    else if (eCurrentProcess == CS_CON || eCurrentProcess == CS_CALC) {
        //if Con  || Calc
        PIFINFO* pPifInfo = new PIFINFO;
        pPifInfo->eType = FILE_NONE;
        pPifInfo->sUName = INPUTTBD;
        if (bUseStateInfo) {
            if (eCurrentProcess == CS_CON) {
                if (!oldPFFile.GetConInputFilenames().empty()) {
                    pPifInfo->sFileName = oldPFFile.GetConInputFilenames().front();
                }
            }
            else if (eCurrentProcess == CS_CALC) {
                if (!oldPFFile.GetCalcInputFNamesArr().empty()) {
                    pPifInfo->sFileName = oldPFFile.GetCalcInputFNamesArr().front();
                }
            }
        }
        pPifInfo->sDisplay = INPUTTBD;
        pPifInfo->uOptions = PIF_FILE_MUST_EXIST | PIF_MULTIPLE_FILES | PIF_READ_ONLY;
        m_arrPifInfo.Add(pPifInfo);

        //output tbd
        pPifInfo = new PIFINFO;
        pPifInfo->eType = FILE_NONE;
        pPifInfo->sUName = OUTPUTTBD;
        if( bUseStateInfo )
            pPifInfo->sFileName = oldPFFile.GetConOutputFName();
        pPifInfo->sDisplay = OUTPUTTBD;
        pPifInfo->uOptions = 0;
        pPifInfo->sDefaultFileExtension = FileExtensions::BinaryTable::Tab;
        m_arrPifInfo.Add(pPifInfo);

        pPifInfo = new PIFINFO;
        if (bUseStateInfo && !oldPFFile.GetListingFName().IsEmpty()) {
            pPifInfo->sFileName = oldPFFile.GetListingFName();
        }
        else {
            pPifInfo->sFileName = m_pPifFile->GetAppFName();
            pPifInfo->sFileName = MakeOutputFileForProcess(eCurrentProcess, pPifInfo->sFileName, _T(""), FileExtensions::Listing);
        }
        //Now do the listing file
        pPifInfo->eType = FILE_NONE;
        pPifInfo->sUName = LISTFILE;
        //pPifInfo->sFileName  = oldPFFile.GetListingFName();
        pPifInfo->sDisplay = LISTFILE;
        pPifInfo->uOptions = 0;
        m_arrPifInfo.Add(pPifInfo);

    }
    else if (eCurrentProcess == CS_PREP) {
        //if Con  || Calc
        PIFINFO* pPifInfo = new PIFINFO;
        pPifInfo->eType = FILE_NONE;
        pPifInfo->sUName = INPUTTBD;
        if( bUseStateInfo )
            pPifInfo->sFileName = oldPFFile.GetPrepInputFName(); //Get the input TBD from the pff file
        pPifInfo->sDisplay = INPUTTBD;
        pPifInfo->uOptions = PIF_FILE_MUST_EXIST | PIF_READ_ONLY;
        m_arrPifInfo.Add(pPifInfo);

        if (bHasArea) { //Add this when the flag for areanames goes into the app file
            pPifInfo = new PIFINFO;
            pPifInfo->eType = FILE_NONE;
            pPifInfo->sUName = AREANAMES;
            if( bUseStateInfo )
                pPifInfo->sFileName = oldPFFile.GetAreaFName();
            pPifInfo->sDisplay = AREANAMES;
            pPifInfo->uOptions = PIF_FILE_MUST_EXIST | PIF_READ_ONLY | PIF_ALLOW_BLANK;
            m_arrPifInfo.Add(pPifInfo);
        }
        //Output TBW
        pPifInfo = new PIFINFO;
        pPifInfo->eType = FILE_NONE;
        pPifInfo->sUName = OUTPUTTBW;

        if( bUseStateInfo )
            pPifInfo->sFileName = oldPFFile.GetPrepOutputFName();
        else
            pPifInfo->sFileName = m_pPifFile->GetAppFName() += FileExtensions::WithDot::Table;

        pPifInfo->sDisplay = OUTPUTTBW;
        pPifInfo->sDefaultFileExtension = FileExtensions::Table;
        pPifInfo->uOptions = 0;
        m_arrPifInfo.Add(pPifInfo);

        //Now do the listing file
        pPifInfo = new PIFINFO;
        if (bUseStateInfo && !oldPFFile.GetListingFName().IsEmpty()) {
            pPifInfo->sFileName = oldPFFile.GetListingFName();
        }
        else {
            pPifInfo->sFileName = m_pPifFile->GetAppFName();
            pPifInfo->sFileName = MakeOutputFileForProcess(eCurrentProcess, pPifInfo->sFileName, _T(""), FileExtensions::Listing);
        }
        pPifInfo->eType = FILE_NONE;
        pPifInfo->sUName = LISTFILE;
        //ppifInfo->sFileName  = oldPFFile.GetListingFName();
        pPifInfo->sDisplay = LISTFILE;
        pPifInfo->uOptions = 0;
        m_arrPifInfo.Add(pPifInfo);

    }
    else {
        AfxMessageBox(_T("Invalid Process Type"));
        return false;
    }

    m_pPifFile->SetTabProcess(eCurrentProcess);

    return true;
}

UINT PifDlgBuilder::ShowModalDialog()
{
    CPifDlg pifDlg(m_arrPifInfo, _T("Define Tab File Associations"));
    pifDlg.m_pPifFile = m_pPifFile;
    return pifDlg.DoModal();
}

bool PifDlgBuilder::SaveAssociations()
{
    PROCESS eCurrentProcess = m_pPifFile->GetTabProcess();
    switch (eCurrentProcess) {
        case ALL_STUFF:
            SaveALLAssociations();
            break;
        case CS_TAB:
            SaveTabAssociations();
            break;
        case CS_CON:
            SaveConAssociations();
            break;
        case CS_CALC:
            ASSERT(FALSE);//calc and prep steps are combined now . unless decided otherwise
            break;
        case CS_PREP:
            SavePrepAssociations();
            break;
        default:
            break;
    }
    if (!CheckFiles()) {
        return false;
    }
    return true;
}

bool PifDlgBuilder::SaveTabAssociations()
{
    bool bRet = true;

    for (int iIndex = 0; iIndex < m_arrPifInfo.GetSize(); iIndex++) {
        PIFINFO* pifInfo = m_arrPifInfo.GetAt(iIndex);

        if (pifInfo->eType == FILE_NONE) {
            if (pifInfo->sUName.CompareNoCase(OUTPUTTBD) == 0) {
                m_pPifFile->SetTabOutputFName(pifInfo->sFileName);
            }
            else if (pifInfo->sUName.CompareNoCase(LISTFILE) == 0) {
                if (pifInfo->sFileName.IsEmpty()) {
                    //Set the default listing file name
                    pifInfo->sFileName = MakeOutputFileForProcess(m_pPifFile->GetTabProcess(), _T(""), FileExtensions::Listing);
                }
                m_pPifFile->SetListingFName(pifInfo->sFileName);
            }
        }
    }

    return bRet;
}

bool PifDlgBuilder::SaveConAssociations()
{
    bool bRet = true;

    for (int iIndex = 0; iIndex < m_arrPifInfo.GetSize(); iIndex++) {
        PIFINFO* pifInfo = m_arrPifInfo.GetAt(iIndex);
        if (pifInfo->eType == FILE_NONE) {
            if (pifInfo->sUName.CompareNoCase(OUTPUTTBD) == 0) {
                m_pPifFile->SetConOutputFName(pifInfo->sFileName);
                m_pPifFile->SetTabOutputFName(pifInfo->sFileName);
            }
            else if (pifInfo->sUName.CompareNoCase(LISTFILE) == 0) {
                if (pifInfo->sFileName.IsEmpty()) {
                    //Set the default listing file name
                    pifInfo->sFileName = MakeOutputFileForProcess(m_pPifFile->GetTabProcess(), _T(""), FileExtensions::Listing);
                }
                m_pPifFile->SetListingFName(pifInfo->sFileName);
            }
        }
    }

    return bRet;
}

bool PifDlgBuilder::SavePrepAssociations()
{
    //output of calc is input of prep
    //Fix Prep takes calc input file names .
    bool bRet = true;

    for (int iIndex = 0; iIndex < m_arrPifInfo.GetSize(); iIndex++) {
        PIFINFO* pifInfo = m_arrPifInfo.GetAt(iIndex);
        if (pifInfo->eType == FILE_NONE) {

            if (pifInfo->sUName.CompareNoCase(INPUTTBD) == 0) {
                if (!m_pPifFile->GetSilent()) {
                    m_pPifFile->SetPrepInputFName(pifInfo->sFileName);
                }

                CIMSAString sCalcOutPutTBD = pifInfo->sFileName;

                PathRemoveExtension(sCalcOutPutTBD.GetBuffer(_MAX_PATH));
                sCalcOutPutTBD.ReleaseBuffer();
                sCalcOutPutTBD += _T(".calc.tab");

                m_pPifFile->SetTabOutputFName(sCalcOutPutTBD);//calc driver uses this name instead of calcoutputFname
                m_pPifFile->SetCalcOutputFName(sCalcOutPutTBD);
            }
            else if (pifInfo->sUName.CompareNoCase(OUTPUTTBW) == 0) {
                m_pPifFile->SetPrepOutputFName(pifInfo->sFileName);
            }
            else if (pifInfo->sUName.CompareNoCase(LISTFILE) == 0) {
                if (pifInfo->sFileName.IsEmpty()) {
                    //Set the default listing file name
                    pifInfo->sFileName = MakeOutputFileForProcess(m_pPifFile->GetTabProcess(), _T(""), FileExtensions::Listing);
                }
                m_pPifFile->SetListingFName(pifInfo->sFileName);
            }
            else if (pifInfo->sUName.CompareNoCase(AREANAMES) == 0) {
                m_pPifFile->SetAreaFName(pifInfo->sFileName);
            }
        }
    }
    return bRet;
}

bool PifDlgBuilder::SaveALLAssociations()
{
    bool bRet = true;

    for (int iIndex = 0; iIndex < m_arrPifInfo.GetSize(); iIndex++) {
        PIFINFO* pifInfo = m_arrPifInfo.GetAt(iIndex);

        if( pifInfo->eType == FILE_NONE ) {
            if (pifInfo->sUName.CompareNoCase(OUTPUTTBD) == 0) {
                m_pPifFile->SetTabOutputFName(pifInfo->sFileName);
            }
            else if (pifInfo->sUName.CompareNoCase(OUTPUTTBW) == 0) {
                m_pPifFile->SetPrepOutputFName(pifInfo->sFileName);
            }
            else if (pifInfo->sUName.CompareNoCase(LISTFILE) == 0) {
                if (pifInfo->sFileName.IsEmpty()) {
                    //Set the default listing file name
                    pifInfo->sFileName = MakeOutputFileForProcess(m_pPifFile->GetTabProcess(), _T(""), FileExtensions::Listing);
                }
                m_pPifFile->SetListingFName(pifInfo->sFileName);
            }
            else if (pifInfo->sUName.CompareNoCase(AREANAMES) == 0) {
                m_pPifFile->SetAreaFName(pifInfo->sFileName);
            }
        }
    }

    return bRet;
}

bool PifDlgBuilder::CheckFiles()
{
    CIMSAString sMsg;
    bool bRet = false;
    CFileStatus fStatus;
    CIMSAString sOutPutFile;
    for (int iIndex = 0; iIndex < m_arrPifInfo.GetSize(); iIndex++) {
        PIFINFO* pifInfo = m_arrPifInfo.GetAt(iIndex);
        if (pifInfo->eType == FILE_NONE) {
            if (pifInfo->sUName.CompareNoCase(OUTPUTTBD) == 0) {
                CFileStatus fStatus;
                CIMSAString sAppName = AfxGetAppName();
                sAppName = sAppName.Left(5);
                bool bAskOverWrite = true;
                if (sAppName.CompareNoCase(_T("CSTab")) == 0) {
                    bAskOverWrite = !m_pPifFile->GetSilent();
                }
                if (bAskOverWrite && CFile::GetStatus(pifInfo->sFileName, fStatus)) {
                    CIMSAString sMsg;
                    sMsg.Format(_T("Output file %s already exists.\nDo you want to replace it?"), (LPCTSTR)pifInfo->sFileName);
                    if (AfxMessageBox(sMsg, MB_YESNO) == IDNO) {
                        return false;
                    }
                }
            }
            else if (pifInfo->sUName.CompareNoCase(OUTPUTTBW) == 0) {
                //if tbw exits ask if it should overwrite
                CFileStatus fStatus;
                CIMSAString sAppName = AfxGetAppName();
                sAppName = sAppName.Left(5);
                bool bAskOverWrite = true;
                if (sAppName.CompareNoCase(_T("CSTab")) == 0) {
                    bAskOverWrite = !m_pPifFile->GetSilent();
                }
                else if (sAppName.CompareNoCase(_T("CSPro")) == 0 && m_pPifFile->GetTabProcess() == ALL_STUFF) {
                    bAskOverWrite = false;
                }
                if (bAskOverWrite && CFile::GetStatus(pifInfo->sFileName, fStatus)) {
                    CIMSAString sMsg;
                    sMsg.Format(_T("Output file %s already exists.\nDo you want to replace it?"), (LPCTSTR)pifInfo->sFileName);
                    if (AfxMessageBox(sMsg, MB_YESNO) == IDNO) {
                        return false;
                    }
                }
            }
        }
    }
    sMsg.IsEmpty() ? bRet = true : bRet = false;
    if (!sMsg.IsEmpty()) {
        AfxMessageBox(sMsg);
    }
    return bRet;
}

CString PifDlgBuilder::MakeOutputFileForProcess(PROCESS eCurrentProcess, const CString& sInputFile,
    const CString& sOutputFolder, CString sType /*= ""*/)
{
    CString sLstExt, sRet, sTabExt;
    //Strip the tab from the inputfile
    if (eCurrentProcess == CS_TAB) {
        sLstExt = CString(FileExtensions::BinaryTable::WithDot::Tab) + FileExtensions::WithDot::Listing;
        sTabExt = FileExtensions::BinaryTable::WithDot::Tab;
    }
    else if (eCurrentProcess == CS_CON) {
        sLstExt = CString(_T(".con")) + FileExtensions::WithDot::Listing;
        sTabExt = _T(".con.tab");
    }
    else if (eCurrentProcess == CS_CALC) {
        sLstExt = CString(_T(".calc")) + FileExtensions::WithDot::Listing;
        sTabExt = _T(".calc.tab");
    }
    else if (eCurrentProcess == CS_PREP) {
        sLstExt = CString(_T(".fmt")) + FileExtensions::WithDot::Listing;
        //sTabExt=".fmt.tab";
    }
    else if (eCurrentProcess == ALL_STUFF) {
        sLstExt = FileExtensions::WithDot::Listing;
        sTabExt = FileExtensions::BinaryTable::WithDot::Tab;
    }
    CString sOutputFile(sInputFile);
    if (!sOutputFolder.IsEmpty()) {
        // output files go in same folder as TBW
        CString sOutputName(sInputFile);
        PathStripPath(sOutputName.GetBuffer());
        sOutputName.ReleaseBuffer();
        sOutputFile = sOutputFolder + _T("\\") + sOutputName;
    }
    if (sType.CompareNoCase(FileExtensions::Listing) == 0) {
        sRet = sOutputFile; //Check if the extension is xtb?
                            /*PathRemoveExtension(sRet.GetBuffer(_MAX_PATH));
                            sRet.ReleaseBuffer();*/
        sRet += sLstExt;
        //Append sLstExt
    }
    else {
        //Do not remove extension if the Process is CS_TAB
        if (eCurrentProcess == ALL_STUFF || eCurrentProcess == CS_TAB) {
            sRet = sOutputFile;
            sRet += sTabExt;
        }
        else {
            //Remove the extension .tab if the Process is anything else
            sRet = sOutputFile;
            CString sFileExt = _T("");
            int iDot = sRet.ReverseFind('.');
            if (iDot > 0) {
                sFileExt = sRet.Mid(iDot + 1);
                if (sFileExt.CompareNoCase(FileExtensions::BinaryTable::Tab) == 0) {
                    PathRemoveExtension(sRet.GetBuffer(_MAX_PATH));
                    sRet.ReleaseBuffer();
                }
            }
            sRet += sTabExt;
        }
    }
    return sRet;
}

void PifDlgBuilder::DeletePifInfos()
{
    int iCount = m_arrPifInfo.GetSize();
    for (int iIndex = 0; iIndex < iCount; iIndex++) {
        delete m_arrPifInfo[iIndex];
    }
    m_arrPifInfo.RemoveAll();
}
