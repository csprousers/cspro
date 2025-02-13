// FreqDoc.cpp : implementation of the CSFreqDoc class
//

#include "StdAfx.h"
#include "FreqDoc.h"
#include "CSFreq.h"
#include "MainFrm.h"
#include <zUtilO/Filedlg.h>
#include <zUtilO/PathHelpers.h>
#include <zUtilO/Specfile.h>
#include <zJson/JsonSpecFile.h>
#include <Zsrcmgro/Compiler.h>
#include <Zsrcmgro/SrcCode.h>
#include <ZBRIDGEO/DataFileDlg.h>
#include <zFormO/FormFile.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zInterfaceF/BatchLogicViewerDlg.h>
#include <zInterfaceF/LogicSettingsDlg.h>
#include <zBatchF/BatchExecutor.h>
#include <zFreqO/UWM.h>


constexpr const FrequencyPrinterOptions::SortType DefaultSortType = FrequencyPrinterOptions::SortType::ByCode;
const std::vector<const TCHAR*> SortTypeNames = { _T("ValueSet"), _T("Code"), _T("Label"), _T("Freq") };

constexpr const OutputFormat DefaultOutputFormat = OutputFormat::Table;
const std::vector<const TCHAR*> OutputFormatNames = { _T("Table"), _T("HTML"), _T("JSON"), _T("Text"), _T("Excel") };

template<typename T>
T ValueFromText(const std::vector<const TCHAR*>& names, T default_value, wstring_view text)
{
    size_t i = 0;

    for( const TCHAR* name : names )
    {
        if( SO::EqualsNoCase(text, name) )
            return static_cast<T>(i);

        ++i;
    }

    return default_value;
}

FrequencyPrinterOptions::SortType SortTypeFromText(wstring_view text) { return ValueFromText(SortTypeNames, DefaultSortType, text); }

OutputFormat OutputFormatFromText(wstring_view text) { return ValueFromText(OutputFormatNames, DefaultOutputFormat, text); }



/////////////////////////////////////////////////////////////////////////////
// CSFreqDoc

IMPLEMENT_DYNCREATE(CSFreqDoc, CDocument)

BEGIN_MESSAGE_MAP(CSFreqDoc, CDocument)
    ON_COMMAND(ID_FILE_RUN, OnFileRun)
    ON_UPDATE_COMMAND_UI(ID_FILE_RUN, OnUpdateFileRun)
    ON_COMMAND(ID_FILE_SAVE, OnFileSave)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
    ON_COMMAND(ID_TOGGLE, OnToggle)
    ON_UPDATE_COMMAND_UI(ID_TOGGLE, OnUpdateToggle)
    ON_COMMAND(ID_OPTIONS_EXCLUDED, OnOptionsExcluded)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_EXCLUDED, OnUpdateOptionsExcluded)
    ON_COMMAND(ID_OPTIONS_LOGIC_SETTINGS, OnOptionsLogicSettings)
    ON_COMMAND(ID_VIEW_BATCH_LOGIC, OnViewBatchLogic)
    ON_UPDATE_COMMAND_UI(ID_VIEW_BATCH_LOGIC, OnUpdateFileRun)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSFreqDoc construction/destruction

CSFreqDoc::CSFreqDoc()
{
    ClearAllTemps();

    ResetFrequencyPff();
    m_batchmode = false;

    ResetValuesToDefault();

    m_sBaseFilename.Format(_T("%sCSFrqRun%d"), GetTempDirectory().c_str(), GetCurrentProcessId()); // 20140312
}

CSFreqDoc::~CSFreqDoc()
{
}



/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL CSFreqDoc::OnOpenDocument(LPCTSTR lpszPathName)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CSFreqDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CFrqOptionsView* pOptionsView = NULL;
    pFrame ? pOptionsView = pFrame->GetFrqOptionsView() : pOptionsView = NULL;

    m_temporaryDataDictFile.reset();

    DeleteContents();
    SetModifiedFlag(FALSE);
    ResetFrequencyPff();

    m_pDataDict = std::make_shared<CDataDict>();
    m_freqnames.clear();

    CString extension = PortableFunctions::PathGetFileExtension<CString>(lpszPathName);

    if (extension.CompareNoCase(FileExtensions::Pff) == 0) {
        m_batchmode = true;
        m_FreqPiff.SetPifFileName(lpszPathName);
        if (m_FreqPiff.LoadPifFile()) {
            if (OpenSpecFile(m_FreqPiff.GetAppFName(), true)) {
                ((CSFreqApp*) AfxGetApp())->m_iReturnCode = 1;
            }
            else {
                ((CSFreqApp*) AfxGetApp())->m_iReturnCode = 8;
            }
        }
        return TRUE;
    }
    else if (extension.CompareNoCase(FileExtensions::FrequencySpec) == 0) {
        if (OpenSpecFile(lpszPathName, false)) {
            AfxGetApp()->WriteProfileString(_T("Settings"), _T("Last Open"), lpszPathName);
            m_FreqPiff.SetAppFName(lpszPathName);
            m_FreqPiff.SetPifFileName(CString(lpszPathName) + FileExtensions::WithDot::Pff);
            if (m_FreqPiff.LoadPifFile(true)) {
                if (m_FreqPiff.GetAppFName().CompareNoCase(lpszPathName) != 0) {
                    AfxMessageBox(FormatText(_T("Spec files in %s\ndoes not match %s"), m_FreqPiff.GetPifFileName().GetString(), lpszPathName));
                    return FALSE;
                }
            }
        }
        else {
            return FALSE;
        }
    }

    else if( extension.CompareNoCase(FileExtensions::Dictionary) == 0 ||
             extension.CompareNoCase(FileExtensions::Data::CSProDB) == 0 ||
             extension.CompareNoCase(FileExtensions::Data::EncryptedCSProDB) == 0 )
    {
        if( !ProcessDictionarySource(lpszPathName) )
            return FALSE;

        if (!OpenDictFile(false)) {
            return FALSE;
        }

        ResetValuesToDefault();

        AddAllItems();
        AfxGetApp()->WriteProfileString(_T("Settings"),_T("Last Open"), lpszPathName);
    }
    else {
        AfxMessageBox(_T("Invalid file type"));
        return FALSE;
    }

    if(pOptionsView){
        pOptionsView->FromDoc();
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSFreqDoc::OpenDictFile(bool silent)
//
/////////////////////////////////////////////////////////////////////////////////
bool CSFreqDoc::OpenDictFile(bool silent)
{
    ClearAllTemps();

    //  Open data dictionary
    CFileStatus fStatus;
    CFile::GetStatus(m_csDictFileName, fStatus);
    m_tDCFTime = fStatus.m_mtime;

    try
    {
        m_pDataDict = CDataDict::InstantiateAndOpen(m_csDictFileName, silent);
        return true;
    }

    catch( const CSProException& )
    {
        m_pDataDict = std::make_shared<CDataDict>();
        m_csDictFileName.Empty();
        return false;
    }
}


void CSFreqDoc::CloseUponFileNonExistance()
{
    // if a dictionary is deleted or moved while CSFreq is open, this sets objects in a
    // state such that the tool no longer thinks that something is open
    ClearAllTemps();
    SetTitle(_T("Untitled"));
    SetModifiedFlag(FALSE);
    m_freqnames.clear();
    m_csDictFileName.Empty();
    ResetFrequencyPff();
}


// When running a Pff file

/////////////////////////////////////////////////////////////////////////////////
//
//  void CSFreqDoc::RunBatch()
//
/////////////////////////////////////////////////////////////////////////////////
void CSFreqDoc::RunBatch()
{
    m_batchmode = true;
    OnFileRun();
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CSFreqDoc::AddAllItems()
//
/////////////////////////////////////////////////////////////////////////////////
void CSFreqDoc::AddAllItems()
{
    CWaitCursor wait;

    for( const DictLevel& dict_level : m_pDataDict->GetLevels() )
    {
        const CDictRecord* pIdRecord = dict_level.GetIdItemsRec(); // Common Record for the level
        for (int k = 0; k < pIdRecord->GetNumItems(); k++)
        {
            const CDictItem* pItem = pIdRecord->GetItem(k);
            ASSERT(pItem->GetOccurs() == 1 && pItem->GetParentItem() == NULL);
            ASSERT(pItem->AddToTreeFor80());

            if (pItem->HasValueSets())
            {
                FREQUENCIES frq;
                frq.freqnames   = pItem->GetValueSet(0).GetName();
                frq.occ         = -1;
                frq.selected = GetSaveExcludedItems();
                frq.bStats = m_bHasFreqStats;
                frq.bNTiles = m_bHasFreqStats;
                frq.iTiles = 10; //get these vals from interface
                m_freqnames.emplace_back(frq);
            }
            else
            {
                FREQUENCIES frq;
                frq.freqnames   = pItem->GetName();
                frq.occ         = -1;
                frq.selected = GetSaveExcludedItems();
                frq.bStats = m_bHasFreqStats;
                frq.bNTiles = m_bHasFreqStats;
                frq.iTiles = 10; //get these vals from interface
                m_freqnames.emplace_back(frq);
            }
            if (pItem->GetNumValueSets() > 1)
            {
                for( size_t vset = 1; vset < pItem->GetNumValueSets(); ++vset )
                {
                    FREQUENCIES frq;
                    frq.freqnames = pItem->GetValueSet(vset).GetName();
                    frq.occ = -1;
                    frq.selected = GetSaveExcludedItems();
                    frq.bStats = m_bHasFreqStats;
                    frq.bNTiles = m_bHasFreqStats;
                    frq.iTiles = 10; //get these vals from interface
                    m_freqnames.emplace_back(frq);
                }
            }
        }

        for (int j = 0; j < dict_level.GetNumRecords(); j++)
        {
            const CDictRecord* pRecord = dict_level.GetRecord(j);
            for (int k = 0; k < pRecord->GetNumItems(); k++)
            {
                const CDictItem* pItem = pRecord->GetItem(k);

                if( !pItem->AddToTreeFor80() )
                    continue;

                int totocc = pItem->GetOccurs();
                if (pItem->GetParentItem() != NULL)
                    totocc = pItem->GetParentItem()->GetOccurs()*totocc;

                if ( totocc > 1)
                {
                    if (pItem->HasValueSets())
                    {
                        FREQUENCIES frq;
                        frq.freqnames   = pItem->GetValueSet(0).GetName();
                        frq.occ         = 0;
                        frq.selected = GetSaveExcludedItems();
                        frq.bStats = m_bHasFreqStats;
                        frq.bNTiles = m_bHasFreqStats;
                        frq.iTiles = 10; //get these vals from interface
                        m_freqnames.emplace_back(frq);
                    }
                    else
                    {
                        FREQUENCIES frq;
                        frq.freqnames   = pItem->GetName();
                        frq.occ         = 0;
                        frq.selected = GetSaveExcludedItems();
                        frq.bStats = m_bHasFreqStats;
                        frq.bNTiles = m_bHasFreqStats;
                        frq.iTiles = 10; //get these vals from interface
                        m_freqnames.emplace_back(frq);
                    }
                    if (pItem->GetNumValueSets() > 1)
                    {
                        for( size_t vset = 1; vset < pItem->GetNumValueSets(); ++vset )
                        {
                            FREQUENCIES frq;
                            frq.freqnames = pItem->GetValueSet(vset).GetName();
                            frq.occ = 0;
                            frq.selected = GetSaveExcludedItems();
                            frq.bStats = m_bHasFreqStats;
                            frq.bNTiles = m_bHasFreqStats;
                            frq.iTiles = 10; //get these vals from interface
                            m_freqnames.emplace_back(frq);
                        }
                    }
                    for(int occ = 0; occ < totocc; occ++)
                    {
                        if (pItem->HasValueSets())
                        {
                            FREQUENCIES frq;
                            frq.freqnames   = pItem->GetValueSet(0).GetName();
                            frq.occ         = occ+1;
                            frq.selected = GetSaveExcludedItems();
                            frq.bStats = m_bHasFreqStats;
                            frq.bNTiles = m_bHasFreqStats;
                            frq.iTiles = 10; //get these vals from interface
                            m_freqnames.emplace_back(frq);
                        }
                        else
                        {
                            FREQUENCIES frq;
                            frq.freqnames   = pItem->GetName();
                            frq.occ         = occ+1;
                            frq.selected = GetSaveExcludedItems();
                            frq.bStats = m_bHasFreqStats;
                            frq.bNTiles = m_bHasFreqStats;
                            frq.iTiles = 10; //get these vals from interface
                            m_freqnames.emplace_back(frq);
                        }

                        if (pItem->GetNumValueSets() > 1)
                        {
                            for( size_t vset = 1; vset < pItem->GetNumValueSets(); ++vset )
                            {
                                FREQUENCIES frq;
                                frq.freqnames = pItem->GetValueSet(vset).GetName();
                                frq.occ = occ+1;
                                frq.selected = GetSaveExcludedItems();
                                frq.bStats = m_bHasFreqStats;
                                frq.bNTiles = m_bHasFreqStats;
                                frq.iTiles = 10; //get these vals from interface
                                m_freqnames.emplace_back(frq);
                            }
                        }
                        //AddtoNewItemList(pItem,occ+1);
                    }

            //      ITEMS a;
            //      a.pItem = pItem;
            //      a.occ   = 0;
            //      a.selected = false;
            //      m_aItems.Add(a);
                }
                else
                {
                    if (pItem->HasValueSets())
                    {
                        FREQUENCIES frq;
                        frq.freqnames   = pItem->GetValueSet(0).GetName();
                        frq.occ         = -1;
                        frq.selected = GetSaveExcludedItems();
                        frq.bStats = m_bHasFreqStats;
                        frq.bNTiles = m_bHasFreqStats;
                        frq.iTiles = 10; //get these vals from interface
                        m_freqnames.emplace_back(frq);
                    }
                    else
                    {
                        FREQUENCIES frq;
                        frq.freqnames   = pItem->GetName();
                        frq.occ         = -1;
                        frq.selected = GetSaveExcludedItems();
                        frq.bStats = m_bHasFreqStats;
                        frq.bNTiles = m_bHasFreqStats;
                        frq.iTiles = 10; //get these vals from interface
                        m_freqnames.emplace_back(frq);
                    }
                    if (pItem->GetNumValueSets() > 1)
                    {
                        for( size_t vset = 1; vset < pItem->GetNumValueSets(); ++vset )
                        {
                            FREQUENCIES frq;
                            frq.freqnames = pItem->GetValueSet(vset).GetName();
                            frq.occ = -1;
                            frq.selected = GetSaveExcludedItems();
                            frq.bStats = m_bHasFreqStats;
                            frq.bNTiles = m_bHasFreqStats;
                            frq.iTiles = 10; //get these vals from interface
                            m_freqnames.emplace_back(frq);
                        }
                    }
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CSFreqDoc::ClearAllTemps()
//
/////////////////////////////////////////////////////////////////////////////////
void CSFreqDoc::ClearAllTemps()
{
    m_pDataDict = std::make_shared<CDataDict>();

    m_logicSettings = LogicSettings::GetUserDefaultSettings();
}


bool CSFreqDoc::RemoveInvalidFrequencyEntries()
{
    size_t initial_size = m_freqnames.size();

    for( auto freqname_itr = m_freqnames.begin(); freqname_itr != m_freqnames.end(); )
    {
        const CDictItem* dict_item;

        if( !m_pDataDict->LookupName(freqname_itr->freqnames, nullptr, nullptr, &dict_item, nullptr) || dict_item == nullptr )
            freqname_itr = m_freqnames.erase(freqname_itr);

        else
            ++freqname_itr;
    }

    // return true if invalid entries have been removed
    return ( initial_size != m_freqnames.size() );
}


void CSFreqDoc::OnFileRun()
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CFrqOptionsView* pOptionsView = NULL;

    if(pFrame) {
        pOptionsView = pFrame->GetFrqOptionsView();
        pOptionsView->ToDoc();
    }

    if(pOptionsView){
        if(!pOptionsView->CheckUniverseSyntax(m_sUniverse)){
            return;
        }
        if(!pOptionsView->CheckWeightSyntax(m_sWeight)){
            return;
        }
    }

    RemoveInvalidFrequencyEntries();

    if( !ExecuteFileInfo() )
        return;

    GenerateBchForFrq();

    LaunchBatch();
}

void CSFreqDoc::OnUpdateFileRun(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!GetPathName().IsEmpty() &&
                   !m_freqnames.empty() &&
                   IsAtLeastOneItemSelected());
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSFreqDoc::IsAtLeastOneItemSelected() const
//
/////////////////////////////////////////////////////////////////////////////////
bool CSFreqDoc::IsAtLeastOneItemSelected() const
{
    return ( std::find_if(m_freqnames.cbegin(), m_freqnames.cend(),
             [](const auto& freqname) { return freqname.selected; }) != m_freqnames.cend() );
}


void CSFreqDoc::ResetValuesToDefault()
{
    m_itemSerialization = ItemSerialization::Included;
    m_bUseVset = true;
    m_bHasFreqStats = false;
    m_percentiles.reset();
    m_sortOrderAscending = true;
    m_sortType = DefaultSortType;
    m_outputFormat = DefaultOutputFormat;
    m_sUniverse.Empty();
    m_sWeight.Empty();

    // set some values from the registry
    auto set_dichotomous = [](auto& value, const TCHAR* key_name, const TCHAR* true_text, auto true_value, auto false_value)
    {
        CString setting = AfxGetApp()->GetProfileString(_T("Settings"), key_name, nullptr);

        if( !setting.IsEmpty() )
            value = ( setting.CompareNoCase(true_text) == 0 ) ? true_value : false_value;
    };

    set_dichotomous(m_itemSerialization, _T("SaveIncluded"), _T("Yes"), ItemSerialization::Included, ItemSerialization::Excluded);
    set_dichotomous(m_bUseVset, _T("TypeValueSet"), _T("Yes"), true, false);
    set_dichotomous(m_bHasFreqStats, _T("GenerateStats"), _T("Yes"), true, false);
    set_dichotomous(m_sortOrderAscending, _T("SortOrder"), _T("Ascending"), true, false);
    m_sortType = SortTypeFromText(AfxGetApp()->GetProfileString(_T("Settings"), _T("SortType"), nullptr));
    m_outputFormat = OutputFormatFromText(AfxGetApp()->GetProfileString(_T("Settings"), _T("OutputFormat"), nullptr));
}



/////////////////////////////////////////////////////////////////////////////////
//
//  int CSFreqDoc::GetPositionInList(CIMSAString name, int occurrence)
//
/////////////////////////////////////////////////////////////////////////////////
int CSFreqDoc::GetPositionInList(wstring_view name, int occurrence, bool reverse_search/* = false*/)
{
    auto check = [&](size_t i)
    {
        return ( SO::EqualsNoCase(name, m_freqnames[i].freqnames) && occurrence == m_freqnames[i].occ );
    };

    if( !reverse_search )
    {
        for( size_t i = 0; i < m_freqnames.size(); ++i )
        {
            if( check(i) )
                return (int)i;
        }
    }

    else // 20111228
    {
        for( size_t i = m_freqnames.size() - 1; i < m_freqnames.size(); i-- )
        {
            if( check(i) )
                return (int)i;
        }
    }

    return -1;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSFreqDoc::CheckValueSetChanges()
//
/////////////////////////////////////////////////////////////////////////////////
bool CSFreqDoc::CheckValueSetChanges()
{
    bool ret = !RemoveInvalidFrequencyEntries();

    for( size_t i = 0; i < m_freqnames.size(); i++ )
    {
        const CDictItem* dict_item;
        const DictValueSet* dict_value_set;
        m_pDataDict->LookupName(m_freqnames[i].freqnames, nullptr, nullptr, &dict_item, &dict_value_set);
        ASSERT(dict_item != nullptr);

        if( m_freqnames[i].occ < 0 && ( ( dict_item->GetOccurs() > 1 ) ||
                                        ( dict_item->GetParentItem() != nullptr && dict_item->GetParentItem()->GetOccurs() > 1 ) ) )
        {
            m_freqnames[i].occ = 0;
            ret = false;
        }

        if( dict_value_set == nullptr && dict_item->HasValueSets() )
        {
            m_freqnames[i].freqnames = dict_item->GetValueSet(0).GetName();
            i--;
            continue;
        }

    }
    return ret;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  CString CSFreqDoc::GetNameat(int level, int record, int item, int vset,int occ)
//
/////////////////////////////////////////////////////////////////////////////////
CString CSFreqDoc::GetNameat(int level, int record, int item, int vset,int occ)
{
    ASSERT (level >= 0);
//  ASSERT (record >= 0);
    //if (occ >0) item = item - occ+1;
    ASSERT (item >= 0);
    ASSERT (vset >= 0);

    const CDictItem* pItem = m_pDataDict->GetLevel(level).GetRecord(( record == -1 ) ? COMMON : record)->GetItem(item);

    if (pItem->HasValueSets())
        return pItem->GetValueSet(vset).GetName();
    else
        return pItem->GetName();
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSFreqDoc::IsChecked(int position)
//
/////////////////////////////////////////////////////////////////////////////////
bool CSFreqDoc::IsChecked(int position) const
{
    return ( position >= 0 && m_freqnames[position].selected );
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSFreqDoc::GenerateBchForFrq(CTabulateDoc* pTabDoc)
//
/////////////////////////////////////////////////////////////////////////////////
bool CSFreqDoc::GenerateBchForFrq()
{
    Application batchApp;
    batchApp.SetEngineAppType(EngineAppType::Batch);
    batchApp.SetLogicSettings(m_logicSettings);

    CString sPathName = this->GetPathName();
    if(sPathName.IsEmpty())
        sPathName = m_FreqPiff.GetPifFileName();

    PathRemoveFileSpec(sPathName.GetBuffer(MAX_PATH));
    sPathName.ReleaseBuffer();

    //make the order spec name and delete existing file
    CString sOrderFile = m_sBaseFilename + FileExtensions::WithDot::Order;
    DeleteFile(sOrderFile);

    batchApp.AddFormFilename(sOrderFile);
    batchApp.AddDictionaryDescription(DictionaryDescription(CS2WS(m_csDictFileName), CS2WS(sOrderFile), DictionaryType::Input));

    //Create the .ord file and save it
    //Create the form if the formfile does not exist
    if(!PortableFunctions::FileIsRegular(sOrderFile)) {
        ASSERT(!m_csDictFileName.IsEmpty());
        CDEFormFile Order(sOrderFile, m_csDictFileName);
        Order.CreateOrderFile(*m_pDataDict, true);
        Order.Save(sOrderFile);
    }

    CString sFullFileName = m_sBaseFilename + FileExtensions::WithDot::BatchApplication;
    batchApp.SetLabel(PortableFunctions::PathGetFilenameWithoutExtension<CString>(sFullFileName));

    CString sAppFile = m_sBaseFilename + FileExtensions::WithDot::Logic;
    DeleteFile(sAppFile);

    CSpecFile appFile(TRUE);
    appFile.Open(sAppFile, CFile::modeWrite);
    appFile.WriteString(GenerateFrqCmd());
    appFile.Close();

    WriteDefaultFiles(&batchApp,sFullFileName);

    try
    {
        batchApp.Save(sFullFileName);
    }

    catch( const CSProException& )
    {
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool  CSFreqDoc::CompileApp()
//
/////////////////////////////////////////////////////////////////////////////////
bool  CSFreqDoc::CompileApp(XTABSTMENT_TYPE eType/* = XTABSTMENT_ALL*/)
{
    if(!IsAtLeastOneItemSelected()){
        AfxMessageBox(_T("You must select at least one item to tabulate\nbefore you set and compile a universe"));
        return false;
    }

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CFrqOptionsView* pOptionsView = pFrame->GetFrqOptionsView();
    pOptionsView->ToDoc();

    RemoveInvalidFrequencyEntries();

    CString sOldUniverse,sOldWeight;
    switch(eType){
        case XTABSTMENT_WGHT_ONLY:
            sOldUniverse = m_sUniverse;
            m_sUniverse=_T("");
            break;
        case XTABSTMENT_UNIV_ONLY:
            sOldWeight = m_sWeight;
            m_sWeight=_T("");
            break;
        case XTABSTMENT_ALL:
        default:
            break;
    }

    if(!GenerateBchForFrq()){
        AfxMessageBox(_T("Failed to generate freq app"));
        if(!sOldUniverse.IsEmpty()){
            m_sUniverse = sOldUniverse;
        }
        if(!sOldWeight.IsEmpty()){
            m_sWeight = sOldWeight;
        }
        return false;
    }
    else {
        if(!sOldUniverse.IsEmpty()){
            m_sUniverse = sOldUniverse;
        }
        if(!sOldWeight.IsEmpty()){
            m_sWeight = sOldWeight;
        }
    }

    GenerateBatchPffFromFrequencyPff();
    m_batchPff->BuildAllObjects();
    ASSERT(m_batchPff->GetApplication());

    try
    {
        CWaitCursor wait;

        if( m_batchPff->GetApplication()->GetCodeFiles().empty() )
            return false;

        CSourceCode srcCode(*m_batchPff->GetApplication());
        m_batchPff->GetApplication()->SetAppSrcCode(&srcCode);
        srcCode.Load();

        std::vector<CString> proc_names = m_batchPff->GetApplication()->GetRuntimeFormFiles().front()->GetOrder();
        srcCode.SetOrder(proc_names);

        CCompiler compiler(m_batchPff->GetApplication());
        CCompiler::Result err = compiler.FullCompile(m_batchPff->GetApplication()->GetAppSrcCode());

        if(err == CCompiler::Result::CantInit || err == CCompiler::Result::NoInit)
            throw CSProException("Cannot init the compiler");

        if (err != CCompiler::Result::NoErrors )
            return false;

        return true;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return false;
    }
}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CSFreqDoc::WriteDefaultFiles(Application* pApplication,const CString& sAppFName)
//
/////////////////////////////////////////////////////////////////////////////////
void CSFreqDoc::WriteDefaultFiles(Application* pApplication,const CString& sAppFName)
{
    //AppFile
    CString sAppSCodeFName(sAppFName);
    PathRemoveExtension(sAppSCodeFName.GetBuffer(_MAX_PATH));
    sAppSCodeFName.ReleaseBuffer();
    sAppSCodeFName += FileExtensions::WithDot::Logic;

    CFileStatus fStatus;
    BOOL bRet = CFile::GetStatus(sAppSCodeFName,fStatus);
    if(!bRet){
        //Create the .app file
        CSpecFile appFile(TRUE);
        appFile.Open(sAppSCodeFName,CFile::modeWrite);
        appFile.WriteString(m_logicSettings.GetDefaultFirstLineForTextSource(pApplication->GetLabel(), AppFileType::Code));
        appFile.Close();
    }

    pApplication->AddCodeFile(CodeFile(CodeType::LogicMain, std::make_shared<TextSource>(CS2WS(sAppSCodeFName))));
}



/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSFreqDoc::RunBatch()
//
/////////////////////////////////////////////////////////////////////////////////
namespace
{
    class GetUniverseAndWeightCallback : public UWMCallback
    {
    public:
        GetUniverseAndWeightCallback(const CSFreqDoc* document)
            :   m_document(document)
        {
        }

        LONG ProcessMessage(WPARAM wParam, LPARAM /*lParam*/) override
        {
            std::tuple<std::wstring, std::wstring>& universe_and_weight = *reinterpret_cast<std::tuple<std::wstring, std::wstring>*>(wParam);
            std::get<0>(universe_and_weight) = m_document->m_sUniverse;
            std::get<1>(universe_and_weight) = m_document->m_sWeight;
            return 1;
        }

    private:
        const CSFreqDoc* m_document;
    };
}


void CSFreqDoc::LaunchBatch()
{
    try
    {
        GenerateBatchPffFromFrequencyPff();
        m_batchPff->Save();

        BatchExecutor batch_executor;
        batch_executor.AddUWMCallback(UWM::Freq::GetUniverseAndWeight, std::make_shared<GetUniverseAndWeightCallback>(this));
        batch_executor.Run(m_batchPff->GetPifFileName());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  CString CSFreqDoc::GenerateFrqCmd()
//
/////////////////////////////////////////////////////////////////////////////////
namespace
{
    const CDictItem* GetParentItemIfRepeating(const CDictItem* item)
    {
        return ( item->GetItemSubitemOccurs() > 1 && item->GetOccurs() == 1 ) ? item->GetParentItem() :
                                                                                item;
    }

    struct SelectedFrequencies
    {
        const DictLevel* level;
        const CDictRecord* record;
        const CDictItem* item;
        const DictValueSet* value_set;
        std::optional<unsigned> occurrence;
    };

    class SelectedFrequenciesWorker
    {
    public:
        SelectedFrequenciesWorker(const CDataDict& dictionary, const CFreqDDTreeCtrl* dictionary_tree)
            :   m_dictionary(dictionary),
                m_dictionaryTree(dictionary_tree),
                m_level(nullptr),
                m_record(nullptr),
                m_item(nullptr),
                m_dictValueSet(nullptr)
        {
        }

        std::vector<std::vector<SelectedFrequencies>> GenerateByFrequencyGroup()
        {
            ProcessLevel(m_dictionaryTree->GetChildItem(m_dictionaryTree->GetRootItem()));

            // group the selected frequencies by a group that can be passed to the frequency command
            std::vector<std::vector<SelectedFrequencies>> grouped_selected_frequencies;

            const SelectedFrequencies* previous_frequency_added = nullptr;

            for( const SelectedFrequencies& selected_frequency : m_selectedFrequencies )
            {
                bool create_new_group = false;

                // create a new group if this is the first frequency
                if( previous_frequency_added == nullptr )
                    create_new_group = true;

                // create a new group if on a different record
                else if( previous_frequency_added->record != selected_frequency.record )
                    create_new_group = true;

                // create a new group if there is an occurrence change (a occurrence when there was none or an occurrence for a different item)
                else if( ( previous_frequency_added->occurrence.has_value() != selected_frequency.occurrence.has_value() ) ||
                         ( ( selected_frequency.occurrence.has_value() &&
                             GetParentItemIfRepeating(previous_frequency_added->item) != GetParentItemIfRepeating(selected_frequency.item) ) ) )
                {
                    create_new_group = true;
                }

                if( create_new_group )
                    grouped_selected_frequencies.emplace_back();

                grouped_selected_frequencies.back().emplace_back(selected_frequency);

                previous_frequency_added = &selected_frequency;
            }

            return grouped_selected_frequencies;
        }

    private:
        template<typename T>
        bool CheckIconIndex(HTREEITEM hTreeItem)
        {
            int image;
            int selected_image;
            m_dictionaryTree->GetItemImage(hTreeItem, image, selected_image);

            if constexpr(std::is_same_v<T, DictLevel>)
            {
                return ( image == 1 );
            }

            else if constexpr(std::is_same_v<T, CDictRecord>)
            {
                return ( image == 2 || image == 3 );
            }

            else if constexpr(std::is_same_v<T, CDictItem>)
            {
                return ( image == 4 || image == 6 || image == 7 );
            }

            else if constexpr(std::is_same_v<T, DictValueSet>)
            {
                return ( image == 5 );
            }

            else
            {
                static_assert_false();
            }
        }

        void ProcessLevel(HTREEITEM hTreeItemLevel)
        {
            ASSERT(CheckIconIndex<DictLevel>(hTreeItemLevel));
            int level_number = 0;

            while( hTreeItemLevel != nullptr )
            {
                m_level = &m_dictionary.GetLevel(level_number);

                ProcessRecord(m_dictionaryTree->GetChildItem(hTreeItemLevel));

                hTreeItemLevel = m_dictionaryTree->GetNextItem(hTreeItemLevel, TVGN_NEXT);
                ++level_number;
            }
        }

        void ProcessRecord(HTREEITEM hTreeItemRecord)
        {
            ASSERT(CheckIconIndex<CDictRecord>(hTreeItemRecord));
            int record_number = -1;

            while( hTreeItemRecord != nullptr )
            {
                m_record = ( record_number == -1 ) ? m_level->GetIdItemsRec() :
                                                     m_level->GetRecord(record_number);

                ProcessItem(m_dictionaryTree->GetChildItem(hTreeItemRecord));

                hTreeItemRecord = m_dictionaryTree->GetNextItem(hTreeItemRecord, TVGN_NEXT);
                ++record_number;
            }
        }

        void ProcessItem(HTREEITEM hTreeItemItem)
        {
            ASSERT(CheckIconIndex<CDictItem>(hTreeItemItem));

            for( int item_number = 0; hTreeItemItem != nullptr; ++item_number )
            {
                m_item = m_record->GetItem(item_number);

                if( !m_item->AddToTreeFor80() )
                    continue;

                // where there are item/subitem occurrences, there will first be a set for "all occurrences"
                // and then a set for each occurrence
                HTREEITEM hTreeItemItemOccurrence = hTreeItemItem;

                m_occurrence.reset();
                ProcessOccurrence(hTreeItemItemOccurrence);

                if( m_item->GetItemSubitemOccurs() > 1 )
                {
                    hTreeItemItemOccurrence = m_dictionaryTree->GetChildItem(hTreeItemItemOccurrence);

                    // to get to the first occurrence set, move past any value sets
                    while( CheckIconIndex<DictValueSet>(hTreeItemItemOccurrence) )
                        hTreeItemItemOccurrence = m_dictionaryTree->GetNextItem(hTreeItemItemOccurrence, TVGN_NEXT);

                    ASSERT(CheckIconIndex<CDictItem>(hTreeItemItemOccurrence));

                    for( unsigned occurrences = 0; occurrences < m_item->GetItemSubitemOccurs(); ++occurrences )
                    {
                        ASSERT(hTreeItemItemOccurrence != nullptr);

                        m_occurrence = occurrences;
                        ProcessOccurrence(hTreeItemItemOccurrence);

                        hTreeItemItemOccurrence = m_dictionaryTree->GetNextItem(hTreeItemItemOccurrence, TVGN_NEXT);
                    }
                }

                hTreeItemItem = m_dictionaryTree->GetNextItem(hTreeItemItem, TVGN_NEXT);
            }
        }

        void ProcessOccurrence(HTREEITEM hTreeItemItemOccurrence)
        {
            ASSERT(CheckIconIndex<CDictItem>(hTreeItemItemOccurrence));

            if( m_item->GetNumValueSets() < 2 )
            {
                m_dictValueSet = m_item->GetFirstValueSetOrNull();
                ProcessValueSet(hTreeItemItemOccurrence);
            }

            // when there are multiple value sets, each value set gets an entry
            else
            {
                HTREEITEM hTreeItemValueSet = m_dictionaryTree->GetChildItem(hTreeItemItemOccurrence);

                for( const DictValueSet& dict_value_set : m_item->GetValueSets() )
                {
                    ASSERT(CheckIconIndex<DictValueSet>(hTreeItemValueSet));

                    m_dictValueSet = &dict_value_set;
                    ProcessValueSet(hTreeItemValueSet);

                    hTreeItemValueSet = m_dictionaryTree->GetNextItem(hTreeItemValueSet, TVGN_NEXT);
                }
            }
        }

        void ProcessValueSet(HTREEITEM hTreeItemItemOrValueSet)
        {
            ASSERT(CheckIconIndex<CDictItem>(hTreeItemItemOrValueSet) || CheckIconIndex<DictValueSet>(hTreeItemItemOrValueSet));

            int item_state = m_dictionaryTree->GetItemState(hTreeItemItemOrValueSet, TVIS_STATEIMAGEMASK) >> 12;

            if( item_state != 1 )
            {
                m_selectedFrequencies.emplace_back(
                    SelectedFrequencies
                    {
                        m_level,
                        m_record,
                        m_item,
                        m_dictValueSet,
                        m_occurrence
                    });
            }
        }

    private:
        const CDataDict& m_dictionary;
        const CFreqDDTreeCtrl* m_dictionaryTree;
        std::vector<SelectedFrequencies> m_selectedFrequencies;
        const DictLevel* m_level;
        const CDictRecord* m_record;
        const CDictItem* m_item;
        const DictValueSet* m_dictValueSet;
        std::optional<unsigned> m_occurrence;
    };
}


CString CSFreqDoc::GenerateFrqCmd()
{
    CWaitCursor wait;
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    ASSERT(pFrame);

    CFrqOptionsView* pOptionsView = pFrame->GetFrqOptionsView();
    pOptionsView->ToDoc();

    CSFreqView* pFrqTreeView = pFrame->GetFreqTreeView();
    CFreqDDTreeCtrl* pDictTree = pFrqTreeView->GetDictTree();


    // generate the optional commands
    std::vector<CString> extra_commands;

    m_sUniverse.Trim();

    if( !m_sUniverse.IsEmpty() )
        extra_commands.emplace_back(FormatText(_T("universe(%s)"), m_sUniverse.GetString()));

    m_sWeight.Trim();

    if( !m_sWeight.IsEmpty() )
    {
        extra_commands.emplace_back(FormatText(_T("weight(%s)"), m_sWeight.GetString()));

        // if the weight is a constant number with decimals, apply the decimal setting
        if( StringToNumber(m_sWeight) != DEFAULT )
        {
            int decimal_pos = m_sWeight.Find(_T('.'));

            if( decimal_pos >= 0 )
            {
                int number_decimals = m_sWeight.GetLength() - decimal_pos - 1;
                extra_commands.emplace_back(FormatText(_T("decimals(%d)"), std::min(number_decimals, 5)));
            }
        }
    }

    if( m_bHasFreqStats )
    {
        extra_commands.emplace_back(_T("stat"));

        if( m_percentiles.has_value() )
            extra_commands.emplace_back(FormatText(_T("percentiles(%d)"), *m_percentiles));
    }

    if( !m_sortOrderAscending || m_sortType != DefaultSortType )
    {
        auto& sort_command = extra_commands.emplace_back(_T("sort("));

        if( !m_sortOrderAscending )
            sort_command.Append(_T("descending "));

        sort_command.AppendFormat(_T("by %s)"), SO::ToLower(SortTypeNames[(size_t)m_sortType]).c_str());
    }

     extra_commands.emplace_back(_T("nonetpercents"));

     if( !m_bUseVset )
         extra_commands.emplace_back(_T("distinct"));


    // see what frequencies must be generated
    std::vector<std::vector<SelectedFrequencies>> grouped_selected_frequencies =
        SelectedFrequenciesWorker(*m_pDataDict, pDictTree).GenerateByFrequencyGroup();
    ASSERT(!grouped_selected_frequencies.empty());

    CString freq_command = _T("PROC GLOBAL\n");
    const DictLevel* last_level_added = nullptr;
    const CDictRecord* last_record_added = nullptr;
    const TCHAR* Tabs[] = { _T("\t"), _T("\t\t"), _T("\t\t\t"), _T("\t\t\t\t") };
    const TCHAR** current_tabs_index = &Tabs[0];

    auto end_record_for_loop = [&]
    {
        if( last_record_added != nullptr && last_record_added->GetMaxRecs() > 1 )
            freq_command.Append(_T("\n\tendfor;\n"));

        last_record_added = nullptr;
    };

    for( const std::vector<SelectedFrequencies>& grouped_selected_frequency : grouped_selected_frequencies )
    {
        ASSERT(!grouped_selected_frequency.empty());
        const SelectedFrequencies& first_selected_frequency_in_group = grouped_selected_frequency.front();

        // add the level procedure if necessary
        if( first_selected_frequency_in_group.level != last_level_added )
        {
            end_record_for_loop();

            freq_command.AppendFormat(_T("\n\nPROC %s\n"), first_selected_frequency_in_group.level->GetName().GetString());
            last_level_added = first_selected_frequency_in_group.level;
        }


        // add a for loop for the record if necessary
        if( first_selected_frequency_in_group.record != last_record_added )
        {
            end_record_for_loop();

            if( first_selected_frequency_in_group.record->GetMaxRecs() > 1 )
            {
                freq_command.AppendFormat(_T("\n\tfor numeric csfreq_record_occurrence in %s_EDT do\n"),
                                          first_selected_frequency_in_group.record->GetName().GetString());
                current_tabs_index = &Tabs[1];
            }

            else
                current_tabs_index = &Tabs[0];

            last_record_added = first_selected_frequency_in_group.record;
        }


        // a routine for adding each of the items in this group
        auto add_items = [&](std::vector<SelectedFrequencies>::const_iterator freq_itr,
                             std::vector<SelectedFrequencies>::const_iterator freq_itr_end)
        {
            freq_command.AppendFormat(_T("\n%sFreq\n%sinclude("), *current_tabs_index, *current_tabs_index);

            std::vector<const DictValueSet*> specified_value_sets;
            const CDictItem* last_item_added = nullptr;

            for( ; freq_itr != freq_itr_end; ++freq_itr )
            {
                const SelectedFrequencies& selected_frequency = *freq_itr;

                if( selected_frequency.value_set != nullptr )
                    specified_value_sets.emplace_back(selected_frequency.value_set);

                // add the item name if not already added
                if( last_item_added != selected_frequency.item )
                {
                    freq_command.AppendFormat(_T("%s%s"),
                        ( last_item_added != nullptr ) ? _T(", ") : _T(""),
                        selected_frequency.item->GetName().GetString());

                    if( selected_frequency.occurrence.has_value() )
                    {
                        freq_command.AppendFormat(_T("(%s%u)"),
                            ( selected_frequency.record->GetMaxRecs() > 1 ) ? _T("*, ") : _T(""),
                            *selected_frequency.occurrence + 1);
                    }

                    last_item_added = selected_frequency.item;
                }
            }

            freq_command.Append(_T(")\n"));


            // add any value sets
            if( !specified_value_sets.empty() )
            {
                freq_command.AppendFormat(_T("%svalueset("), *current_tabs_index);

                const DictValueSet* last_value_set_added = nullptr;

                for( const DictValueSet* value_set : specified_value_sets )
                {
                    ASSERT(last_value_set_added != value_set);

                    freq_command.AppendFormat(_T("%s%s"),
                        ( last_value_set_added != nullptr ) ? _T(", ") : _T(""),
                        value_set->GetName().GetString());

                    last_value_set_added = value_set;
                }

                freq_command.Append(_T(")\n"));
            }


            // add the the extra commands and end the freq command
            for( const CString& extra_command : extra_commands )
                freq_command.AppendFormat(_T("%s%s\n"), *current_tabs_index, extra_command.GetString());

            freq_command.AppendFormat(_T("%s;\n"), *current_tabs_index);

        };



        auto freq_itr = grouped_selected_frequency.cbegin();
        auto freq_itr_end = grouped_selected_frequency.cend();

        if( !first_selected_frequency_in_group.occurrence.has_value() )
            add_items(freq_itr, freq_itr_end);

        // add an item for loop if necessary
        else
        {
            freq_command.AppendFormat(_T("\n%sfor numeric csfreq_item_occurrence in %s000 do\n"), *current_tabs_index,
                                      GetParentItemIfRepeating(first_selected_frequency_in_group.item)->GetName().GetString());
            ++current_tabs_index;

            while( freq_itr != freq_itr_end )
            {
                auto freq_itr_start = freq_itr;

                do
                {
                    ++freq_itr;

                } while( freq_itr != freq_itr_end && freq_itr->occurrence == freq_itr_start->occurrence );

                freq_command.AppendFormat(_T("\n%sif csfreq_item_occurrence = %u then\n"), *current_tabs_index,
                    *freq_itr_start->occurrence + 1);
                ++current_tabs_index;

                add_items(freq_itr_start, freq_itr);

                --current_tabs_index;
                freq_command.AppendFormat(_T("\n%sendif;\n"), *current_tabs_index);
            }

            --current_tabs_index;
            freq_command.AppendFormat(_T("\n%sendfor;\n"), *current_tabs_index);
        }
    }

    end_record_for_loop();

    return freq_command;
}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CSFreqDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CSFreqDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetTitle() != _T("Untitled"));
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSFreqDoc::OnFileSaveAs()
//
/////////////////////////////////////////////////////////////////////////////////
void CSFreqDoc::OnFileSaveAs()
{
    CString csPath = m_FreqPiff.GetAppFName();         // BMD 14 Mar 2002
    if (SO::IsBlank(csPath)) {
        CString csDictionarySourceFilename = GetDictionarySourceFilename();
        csPath = csDictionarySourceFilename.Left(csDictionarySourceFilename.ReverseFind('\\')) + _T("\\*.fqf");
    }

    CString csFilter = _T("Frequency Specification Files (*.fqf)|*.fqf|All Files (*.*)|*.*||");

    CIMSAFileDialog dlgFile(FALSE, FileExtensions::FrequencySpec, csPath, OFN_HIDEREADONLY, csFilter);
    dlgFile.m_ofn.lpstrTitle = _T("Save Frequency Specification File");
    bool bOK = false;
    while (!bOK) {
        if (dlgFile.DoModal() == IDCANCEL) {
            return;
        }

        CFileStatus status;
        if (CFile::GetStatus(dlgFile.GetPathName(), status)) {
            CString csMessage = dlgFile.GetPathName() + _T(" already exists.\nDo you want to replace it?");
            if (AfxMessageBox(csMessage,MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES) {
                continue;
            }
            else {
                bOK = true;
            }
        }
        else {
            bOK = true;
        }
    }
    if (bOK) {
        m_FreqPiff.SetAppFName(dlgFile.GetPathName());
        SaveSpecFile();
        SetModifiedFlag(FALSE);
        AfxGetApp()->AddToRecentFileList(dlgFile.GetPathName());
        SetPathName(m_FreqPiff.GetAppFName(), TRUE);
        m_FreqPiff.SetPifFileName(dlgFile.GetPathName() + FileExtensions::WithDot::Pff);
        m_FreqPiff.Save();     // BMD 14 Mar 2002
        m_bSaved= true;
        return;
    }
    m_bSaved = false;
    return;
}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CSFreqDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CSFreqDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
{
    CString str = GetTitle();
    if (str == _T("Untitled"))
        pCmdUI->Enable(FALSE);
    else
        pCmdUI->Enable(TRUE);
}


BOOL CSFreqDoc::SaveModified()
{
    // borrowed from Bruce::SaveModified(); see doccore.cpp

    if (!IsModified()) {
        return TRUE;        // ok to continue
    }

    CString name = m_FreqPiff.GetAppFName();
    if (name.IsEmpty()) {
        VERIFY(name.LoadString(AFX_IDS_UNTITLED));
    }
    CString prompt;
    AfxFormatString1(prompt, AFX_IDP_ASK_TO_SAVE, name);
    switch (AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE))
    {
    case IDCANCEL:
        return FALSE;       // don't continue

    case IDYES:
        // If so, either Save or Update, as appropriate
        OnFileSave();
        return m_bSaved;

    case IDNO:
        // If not saving changes, revert the document
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return TRUE;    // keep going
}



/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSFreqDoc::OnFileSave()
//
/////////////////////////////////////////////////////////////////////////////////
void CSFreqDoc::OnFileSave()
{
    if (m_FreqPiff.GetAppFName().IsEmpty()) {
        OnFileSaveAs();
    }
    else {
        SaveSpecFile();
        SetModifiedFlag(FALSE);
        m_bSaved =  true;
    }
}


void CSFreqDoc::DoPostRunCleanUp()
{
#ifndef _DEBUG
    //delete the .pff and other files
    if( !m_batchPff->GetPifFileName().IsEmpty() )
    {
        DeleteFile(m_batchPff->GetPifFileName());
        DeleteFile(m_sBaseFilename + FileExtensions::WithDot::BatchApplication);
        DeleteFile(m_sBaseFilename + FileExtensions::WithDot::Order);
        DeleteFile(m_sBaseFilename + FileExtensions::WithDot::Logic);
        DeleteFile(m_sBaseFilename + FileExtensions::WithDot::TableSpec);
    }
#endif
}


bool CSFreqDoc::ExecuteFileInfo()
{
    bool bRet = true;

    CString base_name_for_files = m_FreqPiff.GetAppFName().IsEmpty() ? _T("CSFrqRun") :
        PortableFunctions::PathGetFilenameWithoutExtension<CString>(m_FreqPiff.GetAppFName());

    CString directory_for_files = PathHelpers::GetDirectoryName({ m_FreqPiff.GetAppFName(), GetDictionarySourceFilename() });

    if( m_FreqPiff.GetListingFName().IsEmpty() )
        m_FreqPiff.SetListingFName(PathHelpers::GetFilenameInDirectory(base_name_for_files + FileExtensions::WithDot::Listing, directory_for_files));

    if( m_FreqPiff.GetFrequenciesFilename().IsEmpty() )
        m_FreqPiff.SetFrequenciesFilename(PathHelpers::GetFilenameInDirectory(base_name_for_files + FileExtensions::WithDot::Table, directory_for_files));


    // make sure the output format extension matches the selection
    CString output_format_extension = ( m_outputFormat == OutputFormat::Table ) ?   FileExtensions::Table :
                                      ( m_outputFormat == OutputFormat::HTML )  ?   FileExtensions::HTML :
                                      ( m_outputFormat == OutputFormat::Json )  ?   FileExtensions::Json :
                                      ( m_outputFormat == OutputFormat::Text )  ?   FileExtensions::Listing :
                                    /*( m_outputFormat == OutputFormat::Excel ) ? */FileExtensions::Excel;

    CString current_extension = PortableFunctions::PathGetFileExtension<CString>(m_FreqPiff.GetFrequenciesFilename());

    if( current_extension.CompareNoCase(output_format_extension) != 0 )
    {
        m_FreqPiff.SetFrequenciesFilename(PortableFunctions::PathRemoveFileExtension<CString>(m_FreqPiff.GetFrequenciesFilename())
            + _T(".") + output_format_extension);
    }

    // make sure the frequencies filename isn't the same as the listing filename
    if( m_FreqPiff.GetFrequenciesFilename().CompareNoCase(m_FreqPiff.GetListingFName()) == 0 )
    {
        m_FreqPiff.SetFrequenciesFilename(PortableFunctions::PathRemoveFileExtension<CString>(m_FreqPiff.GetFrequenciesFilename())
            + _T(".freq.") + output_format_extension);
    }


    if( !m_batchmode )
    {
        // don't ask for a data file if a CSPro DB file has been opened
        if( m_temporaryDataDictFile != nullptr )
        {
            m_FreqPiff.SetSingleInputDataConnectionString(m_csDictionarySourceDataFilename);
        }

        else
        {
            DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true, m_FreqPiff.GetInputDataConnectionStringsSerializable());
            data_file_dlg.SetTitle(_T("Select Data File(s) to Tabulate"))
                         .SetDictionaryFilename(m_csDictFileName)
                         .AllowMultipleSelections();

            if( data_file_dlg.DoModal() != IDOK )
                return false;

            m_FreqPiff.ClearAndAddInputDataConnectionStrings(data_file_dlg.GetConnectionStrings());
        }

        // save an updated frequency PFF
        if( !m_FreqPiff.GetPifFileName().IsEmpty() )
            m_FreqPiff.Save();
    }

    return bRet;
}


void CSFreqDoc::OnToggle()
{
    SharedSettings::ToggleViewNamesInTree();

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CSFreqView* pFrqTreeView = pFrame->GetFreqTreeView();
    pFrqTreeView->RefreshTree();
}

void CSFreqDoc::OnUpdateToggle(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(SharedSettings::ViewNamesInTree());
}


void CSFreqDoc::OnOptionsExcluded()
{
    m_itemSerialization = ( m_itemSerialization == ItemSerialization::Excluded ) ? ItemSerialization::Included :
                                                                                   ItemSerialization::Excluded;
    AfxGetApp()->WriteProfileString(_T("Settings"), _T("SaveIncluded"), GetSaveExcludedItems() ? _T("No") : _T("Yes"));
    SetModifiedFlag();
}

void CSFreqDoc::OnUpdateOptionsExcluded(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(GetSaveExcludedItems());
}


void CSFreqDoc::OnOptionsLogicSettings()
{
    LogicSettingsDlg dlg(m_logicSettings);

    if( dlg.DoModal() != IDOK || m_logicSettings == dlg.GetLogicSettings() )
        return;

    m_logicSettings = dlg.GetLogicSettings();
    SetModifiedFlag();

    // refresh the Scintilla lexer
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFrqOptionsView* pOptionsView = pFrame->GetFrqOptionsView();
    pOptionsView->RefreshLexer();
}


void CSFreqDoc::OnViewBatchLogic()
{
    BatchLogicViewerDlg dlg(*m_pDataDict, m_logicSettings, CS2WS(GenerateFrqCmd()));
    dlg.DoModal();
}


bool CSFreqDoc::ProcessDictionarySource(const CString& filename)
{
    std::unique_ptr<CDataDict> embedded_dictionary = DataRepositoryHelpers::GetEmbeddedDictionary(ConnectionString(filename));

    if( embedded_dictionary != nullptr )
    {
        // for now, save the embedded dictionary; ideally this would not need to be saved to the disk
        try
        {
            m_temporaryDataDictFile = std::make_unique<TemporaryFile>();
            embedded_dictionary->Save(m_temporaryDataDictFile->GetPath());
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
            return false;
        }

        m_csDictFileName = WS2CS(m_temporaryDataDictFile->GetPath());
        m_csDictionarySourceDataFilename = filename;
    }

    else
    {
        m_temporaryDataDictFile.reset();
        m_csDictFileName = filename;
    }

    return true;
}


const CString& CSFreqDoc::GetDictionarySourceFilename() const
{
    return ( m_temporaryDataDictFile == nullptr ) ? m_csDictFileName :
                                                    m_csDictionarySourceDataFilename;
}


CString CSFreqDoc::GetDocumentWindowTitle() const
{
    CString csMainFilename = m_FreqPiff.GetAppFName();

    if( csMainFilename.IsEmpty() )
        csMainFilename = GetDictionarySourceFilename();

    CString csDocumentTitle = GetFileName(csMainFilename);

    // add the dictionary name
    if( !csDocumentTitle.IsEmpty() && m_pDataDict != nullptr )
        csDocumentTitle.AppendFormat(_T(" (%s)"), m_pDataDict->GetName().GetString());

    return csDocumentTitle;
}


void CSFreqDoc::ResetFrequencyPff()
{
    m_FreqPiff.ResetContents();
    m_FreqPiff.SetAppType(FREQ_TYPE);
    m_FreqPiff.SetViewListing(ONERROR);
    m_FreqPiff.SetViewResultsFlag(true);
}


void CSFreqDoc::GenerateBatchPffFromFrequencyPff()
{
    // create the batch PFF, basing it on the contents of the frequency PFF
    m_batchPff = std::make_unique<CNPifFile>(m_FreqPiff);
    m_batchPff->SetPifFileName(m_sBaseFilename + FileExtensions::WithDot::Pff);
    m_batchPff->SetAppType(BATCH_TYPE);
    m_batchPff->SetAppFName(m_sBaseFilename + FileExtensions::WithDot::BatchApplication);

    if (m_batchmode && !m_FreqPiff.GetStartLanguageString().IsEmpty())
        m_batchPff->SetStartLanguageString(m_FreqPiff.GetStartLanguageString());
    else
        m_batchPff->SetStartLanguageString(WS2CS(m_pDataDict->GetCurrentLanguage().GetName()));

    // OnExit should only be executed if CSFreq was run with a PFF as a command line argument
    if( !m_batchmode )
        m_batchPff->SetOnExitFilename(_T(""));
}



/////////////////////////////////////////////////////////////////////////////////
//
// Spec file serialization
//
/////////////////////////////////////////////////////////////////////////////////

CREATE_JSON_VALUE(frequencies)

CREATE_ENUM_JSON_SERIALIZER(ItemSerialization,
    { ItemSerialization::Included, _T("included") },
    { ItemSerialization::Excluded, _T("excluded") })

CREATE_ENUM_JSON_SERIALIZER(OutputFormat,
    { OutputFormat::Table, _T("TBW") },
    { OutputFormat::HTML,  _T("HTML") },
    { OutputFormat::Json,  _T("JSON") },
    { OutputFormat::Text,  _T("text") },
    { OutputFormat::Excel, _T("Excel") })


bool CSFreqDoc::OpenSpecFile(const TCHAR* filename, bool silent)
{
    try
    {
        auto json_reader = JsonSpecFile::CreateReader(filename, nullptr, [&]() { return ConvertPre80SpecFile(filename); });

        try
        {
            json_reader->CheckVersion();
            json_reader->CheckFileType(JV::frequencies);

            // open the dictionary
            std::wstring dictionary_filename = json_reader->GetAbsolutePath(JK::dictionary);

            if( !ProcessDictionarySource(WS2CS(dictionary_filename)) || !OpenDictFile(silent) )
                throw CSProException(_T("The dictionary could not be read: %s"), dictionary_filename.c_str());

            // reestablish the dictionary language
            std::optional<wstring_view> language_name = json_reader->GetOptional<wstring_view>(JK::language);

            if( language_name.has_value() )
            {
                std::optional<size_t> language_index = m_pDataDict->IsLanguageDefined(*language_name);

                if( language_index.has_value() )
                {
                    m_pDataDict->SetCurrentLanguage(*language_index);
                }

                else
                {
                    json_reader->LogWarning(_T("The dictionary language '%s' is not in the dictionary '%s'"),
                                            std::wstring(*language_name).c_str(), m_pDataDict->GetName().GetString());
                }
            }

            // read the other properties
            m_outputFormat = json_reader->GetOrDefault(JK::output, DefaultOutputFormat);
            m_bUseVset = json_reader->GetOrDefault(JK::useValueSets, true);
            m_bHasFreqStats = json_reader->GetOrDefault(JK::statistics, false);

            if( m_bHasFreqStats )
            {
                m_percentiles = json_reader->GetOptional<int>(JK::percentiles);

                // validate the percentiles value
                if( m_percentiles.has_value() && ( *m_percentiles < 2 || *m_percentiles > 20 ) )
                {
                    json_reader->LogWarning(_T("The percentiles value '%d' is not valid and been reset"), *m_percentiles);
                    m_percentiles.reset();
                }
            }

            const auto& sort_node = json_reader->GetOrEmpty(JK::sort);
            m_sortOrderAscending = sort_node.GetOrDefault(JK::ascending, true);
            m_sortType = sort_node.GetOrDefault(JK::order, DefaultSortType);

            m_logicSettings = json_reader->GetOrDefault(JK::logicSettings, m_logicSettings);

            m_sUniverse = json_reader->GetOrDefault(JK::universe, SO::EmptyCString);
            m_sWeight = json_reader->GetOrDefault(JK::weight, SO::EmptyCString);

            m_itemSerialization = json_reader->GetOrDefault(JK::itemSerialization, ItemSerialization::Included);

            // create the list of all the items from the dictionary
            AddAllItems();

            // read the items
            for( const auto& item_node : json_reader->GetArrayOrEmpty(JK::items) )
            {
                auto item_name = item_node.GetOptional<wstring_view>(JK::name);

                if( item_name.has_value() )
                {
                    int occurrence = item_node.GetOrDefault(JK::occurrence, -1);
                    int pos = GetPositionInList(*item_name, occurrence);

                    if( pos == -1 )
                    {
                        json_reader->LogWarning(_T("'%s' is not a valid item or value set in the dictionary '%s'"),
                                                std::wstring(*item_name).c_str(), m_pDataDict->GetName().GetString());
                    }

                    else
                    {
                        SetItemCheck(pos, !GetSaveExcludedItems());
                    }
                }
            }

            if( !CheckValueSetChanges() )
            {
                // AfxMessageBox("Some Items have been adjusted for changes in version");
            }
        }

        catch( const CSProException& exception )
        {
            json_reader->GetMessageLogger().RethrowException(filename, exception);
        }

        // update the options
        CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
        CFrqOptionsView* pOptionsView = ( pFrame != nullptr ) ? pFrame->GetFrqOptionsView() : nullptr;

        if( pOptionsView != nullptr )
            pOptionsView->FromDoc();

        // report any warnings
        json_reader->GetMessageLogger().DisplayWarnings(silent);

        return true;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return false;
    }
}


void CSFreqDoc::SaveSpecFile() const
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CFrqOptionsView* pOptionsView = pFrame->GetFrqOptionsView();

    pOptionsView->ToDoc();

    try
    {
        auto json_writer = JsonSpecFile::CreateWriter(m_FreqPiff.GetAppFName(), JV::frequencies);

        json_writer->WriteRelativePath(JK::dictionary, CS2WS(GetDictionarySourceFilename()));

        json_writer->Write(JK::language, m_pDataDict->GetCurrentLanguage().GetName())
                    .Write(JK::output, m_outputFormat)
                    .Write(JK::useValueSets, m_bUseVset)
                    .Write(JK::statistics, m_bHasFreqStats);

        if( m_bHasFreqStats && m_percentiles.has_value() )
            json_writer->Write(JK::percentiles, *m_percentiles);

        json_writer->Key(JK::sort).WriteObject(
            [&]()
            {
                json_writer->Write(JK::ascending, m_sortOrderAscending);
                json_writer->Write(JK::order, m_sortType);
            });

        json_writer->Write(JK::logicSettings, m_logicSettings);

        json_writer->WriteIfNotBlank(JK::universe, m_sUniverse)
                    .WriteIfNotBlank(JK::weight, m_sWeight);

        json_writer->Write(JK::itemSerialization, m_itemSerialization);

        json_writer->BeginArray(JK::items);
        int freqname_index = 0;

        for( const FREQUENCIES& freqname : m_freqnames )
        {
            if( IsChecked(freqname_index++) == GetSaveExcludedItems() )
                continue;

            json_writer->WriteObject(
                [&]()
                {
                    json_writer->Write(JK::name, freqname.freqnames);

                    if( freqname.occ != -1 )
                        json_writer->Write(JK::occurrence, freqname.occ);
                });
        }

        json_writer->EndArray();

        json_writer->EndObject();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


std::wstring CSFreqDoc::ConvertPre80SpecFile(NullTerminatedString filename)
{
    CSpecFile specfile;

    if( !specfile.Open(filename.c_str(), CFile::modeRead) )
        throw CSProException(_T("Failed to open the Tabulate Frequencies specification file: %s"), filename.c_str());

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::version, 7.7);
    json_writer->Write(JK::fileType, JV::frequencies);

    json_writer->Write(JK::logicSettings, LogicSettings::GetOriginalSettings());

    try
    {
        // Is correct spec file?
        if( !specfile.IsHeaderOK(_T("[CSFreq]")) )
            throw CSProException(_T("Spec File does not begin with\n\n    [CSFreq]"));

        // Ignore version errors
        specfile.IsVersionOK(CSPRO_VERSION);

        CString command;
        CString argument;

        bool sort_ascending = true;
        FrequencyPrinterOptions::SortType sort_type = DefaultSortType;

        std::vector<std::tuple<CString, int>> names_and_occurrences;

        while( specfile.GetLine(command, argument) == SF_OK )
        {
            if( command.CompareNoCase(_T("File")) == 0 )
            {
                json_writer->Write(JK::dictionary, specfile.EvaluateRelativeFilename(argument));
            }

            else if( command.CompareNoCase(_T("ItemsAre")) == 0 )
            {
                json_writer->Write(JK::itemSerialization, SO::ToLower(argument));
            }

            else if( command.CompareNoCase(_T("UseVSet")) == 0 )
            {
                json_writer->Write(JK::useValueSets, ( argument.CompareNoCase(_T("Yes")) == 0 ));
            }

            else if( command.CompareNoCase(_T("GenerateStats")) == 0 )
            {
                json_writer->Write(JK::statistics, ( argument.CompareNoCase(_T("Yes")) == 0 ));
            }

            else if( command.CompareNoCase(_T("Percentiles")) == 0 )
            {
                json_writer->Write(JK::percentiles, _ttoi(argument));
            }

            else if( command.CompareNoCase(_T("SortOrder")) == 0 )
            {
                sort_ascending = ( argument.CompareNoCase(_T("Descending")) != 0 );
            }

            else if( command.CompareNoCase(_T("SortType")) == 0 )
            {
                sort_type = SortTypeFromText(argument);
            }

            else if( command.CompareNoCase(_T("OutputFormat")) == 0 )
            {
                json_writer->Write(JK::output, OutputFormatFromText(argument));
            }

            else if( command.CompareNoCase(_T("Universe")) == 0 )
            {
                json_writer->Write(JK::universe, argument);
            }

            else if( command.CompareNoCase(_T("Weight")) == 0 )
            {
                json_writer->Write(JK::weight, argument);
            }

            else if( command.CompareNoCase(_T("Language")) == 0 )
            {
                json_writer->Write(JK::language, argument);
            }

            else if( command.CompareNoCase(_T("[Item]")) == 0 )
            {
                names_and_occurrences.emplace_back(CString(), -1);
            }

            else if( command.CompareNoCase(_T("Name")) == 0 && !names_and_occurrences.empty() )
            {
                std::get<0>(names_and_occurrences.back()) = argument;
            }
                
            else if( command.CompareNoCase(_T("Occ")) == 0 && !names_and_occurrences.empty() )
            {
                if( !argument.IsEmpty() )
                    std::get<1>(names_and_occurrences.back()) = _ttoi(argument);
            }
                
            else if( command.CompareNoCase(_T("[Dictionaries]")) != 0 &&
                     command.CompareNoCase(_T("[Items]")) != 0 &&
                     command.CompareNoCase(_T("[Item]")) != 0 &&
                     command.CompareNoCase(_T("[EndItem]")) != 0 &&
                     command.CompareNoCase(_T("Stats")) != 0 &&
                     command.CompareNoCase(_T("NTiles")) != 0 )
            {
                throw CSProException(_T("Spec File: Invalid command: %s"), command.GetString());
            }
        }

        json_writer->Key(JK::sort).WriteObject(
            [&]()
            {
                json_writer->Write(JK::ascending, sort_ascending);
                json_writer->Write(JK::order, sort_type);
            });

        json_writer->WriteObjects(JK::items, names_and_occurrences,
            [&](const auto& name_and_occurrence)
            {
                json_writer->Write(JK::name, std::get<0>(name_and_occurrence));

                if( std::get<1>(name_and_occurrence) != -1 )
                    json_writer->Write(JK::occurrence, std::get<1>(name_and_occurrence));
            });

        specfile.Close();
    }

    catch( const CSProException& exception )
    {
        specfile.Close();

        throw CSProException(_T("There was an error reading the Tabulate Frequencies specification file %s:\n\n%s"),
                             PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());
    }

    json_writer->EndObject();

    return json_writer->GetString();
}
