#include "StdAfx.h"
#include "FreqDoc.h"
#include "CSFreq.h"
#include "MainFrm.h"
#include <zUtilO/Filedlg.h>
#include <zUtilO/PathHelpers.h>
#include <zUtilO/Specfile.h>
#include <zJson/JsonSpecFile.h>
#include <zSrcMgrO/Compiler.h>
#include <zSrcMgrO/SrcCode.h>
#include <zBridgeO/DataFileDlg.h>
#include <zFormO/FormFile.h>
#include <zInterfaceF/BatchLogicViewerDlg.h>
#include <zInterfaceF/LogicSettingsDlg.h>
#include <zBatchF/BatchExecutor.h>
#include <zFreqO/UWM.h>


constexpr const FrequencyPrinterOptions::SortType DefaultSortType = FrequencyPrinterOptions::SortType::ByCode;
const char* SortTypeNames[] = { "ValueSet", "Code", "Label", "Freq" };

constexpr const OutputFormat DefaultOutputFormat = OutputFormat::Table;
const char* OutputFormatNames[] = { "Table", "HTML", "JSON", "Text", "Excel" };

template<typename T>
T ValueFromText(const char* names[], const size_t names_count, T default_value, const std::string_view text_sv)
{
    for( size_t i = 0; i < names_count; ++i )
    {
        if( SO::EqualsNoCase(text_sv, names[i]) )
            return static_cast<T>(i);

        ++i;
    }

    return default_value;
}

FrequencyPrinterOptions::SortType SortTypeFromText(std::string_view text_sv) { return ValueFromText(SortTypeNames, _countof(SortTypeNames), DefaultSortType, text_sv); }

OutputFormat OutputFormatFromText(std::string_view text_sv) { return ValueFromText(OutputFormatNames, _countof(OutputFormatNames), DefaultOutputFormat, text_sv); }



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
    :   m_baseFilePath(Path::Combine(GetTempDirectory(), "CSFrqRun" + IntToString(static_cast<uint64_t>(GetCurrentProcessId()))))
{
    ClearAllTemps();

    ResetFrequencyPff();
    m_batchmode = false;

    ResetValuesToDefault();
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
    CSFreqApp* const csfreq_app = assert_cast<CSFreqApp*>(AfxGetApp());
    CMainFrame* const pFrame = assert_nullable_cast<CMainFrame*>(AfxGetMainWnd());
    CFrqOptionsView* pOptionsView = pFrame ? pOptionsView = pFrame->GetFrqOptionsView() : NULL;

    DeleteContents();
    SetModifiedFlag(FALSE);
    ResetFrequencyPff();

    m_dictionarySource.Reset();
    m_dictionary = std::make_unique<CDataDict>();
    m_freqnames.clear();

    const ConnectionString connection_string = csfreq_app->GetConnectionStringFileSimulator().GetConnectionString(lpszPathName);
    const std::string extension = connection_string.HasFilePath() ? PortableFunctions::PathGetFileExtension(connection_string.GetFilePath()) :
                                                                    std::string();

    try
    {
        if( SO::EqualsNoCase(extension, FileExtensions::Pff) )
        {
            m_batchmode = true;
            m_FreqPiff.SetPifFileName(UTF8_TODO::GetCString(connection_string.GetFilePath()));
            if (m_FreqPiff.LoadPifFile()) {
                if (OpenSpecFile(UTF8_TODO::GetUtf8(m_FreqPiff.GetAppFName()), true)) {
                    csfreq_app->m_iReturnCode = 1;
                }
                else {
                    csfreq_app->m_iReturnCode = 8;
                }
            }
            return TRUE;
        }

        else if( SO::EqualsNoCase(extension, FileExtensions::FrequencySpec) )
        {
            if (OpenSpecFile(connection_string.GetFilePath(), false)) {
                AfxGetApp()->WriteProfileString(L"Settings", L"Last Open", lpszPathName);
                m_FreqPiff.SetAppFName(UTF8_TODO::GetCString(connection_string.GetFilePath()));
                m_FreqPiff.SetPifFileName(UTF8_TODO::GetCString(Path::AppendExtension(connection_string.GetFilePath(), FileExtensions::Pff)));
                if (m_FreqPiff.LoadPifFile(true)) {
                    if (!SO::EqualsNoCase(connection_string.GetFilePath(), m_FreqPiff.GetAppFName())) {
                        throw CSProException("Spec files in %s\ndoes not match %s", UTF8_TODO::GetUtf8(m_FreqPiff.GetPifFileName()).c_str(), connection_string.GetFilePath().c_str());
                    }
                }
            }
            else {
                return FALSE;
            }
        }

        else
        {
            ProcessDictionarySource(DictionarySource(connection_string));

            ResetValuesToDefault();

            AddAllItems();

            if( connection_string.HasFilePath() )
                AfxGetApp()->WriteProfileString(L"Settings", L"Last Open", lpszPathName);
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }

    if( pOptionsView != nullptr )
        pOptionsView->FromDoc();

    return TRUE;
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

    for( const DictLevel& dict_level : m_dictionary->GetLevels() )
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
                frq.freqnames   = UTF8_TODO::GetCString(pItem->GetValueSet(0).GetName());
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
                frq.freqnames   = UTF8_TODO::GetCString(pItem->GetName());
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
                    frq.freqnames = UTF8_TODO::GetCString(pItem->GetValueSet(vset).GetName());
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
                        frq.freqnames   = UTF8_TODO::GetCString(pItem->GetValueSet(0).GetName());
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
                        frq.freqnames   = UTF8_TODO::GetCString(pItem->GetName());
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
                            frq.freqnames = UTF8_TODO::GetCString(pItem->GetValueSet(vset).GetName());
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
                            frq.freqnames   = UTF8_TODO::GetCString(pItem->GetValueSet(0).GetName());
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
                            frq.freqnames   = UTF8_TODO::GetCString(pItem->GetName());
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
                                frq.freqnames = UTF8_TODO::GetCString(pItem->GetValueSet(vset).GetName());
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
                        frq.freqnames   = UTF8_TODO::GetCString(pItem->GetValueSet(0).GetName());
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
                        frq.freqnames   = UTF8_TODO::GetCString(pItem->GetName());
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
                            frq.freqnames = UTF8_TODO::GetCString(pItem->GetValueSet(vset).GetName());
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
    m_dictionarySource.Reset();
    m_dictionary = std::make_unique<CDataDict>();

    m_logicSettings = LogicSettings::GetUserDefaultSettings();
}


bool CSFreqDoc::RemoveInvalidFrequencyEntries()
{
    const size_t initial_size = m_freqnames.size();

    for( auto freqname_itr = m_freqnames.begin(); freqname_itr != m_freqnames.end(); )
    {
        const CDictItem* dict_item;

        if( !m_dictionary->LookupName(UTF8_TODO::GetUtf8(freqname_itr->freqnames), nullptr, nullptr, &dict_item, nullptr) || dict_item == nullptr )
        {
            freqname_itr = m_freqnames.erase(freqname_itr);
        }

        else
        {
            ++freqname_itr;
        }
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
        if(!pOptionsView->CheckUniverseSyntax(m_universe)){
            return;
        }
        if(!pOptionsView->CheckWeightSyntax(m_weight)){
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
    m_universe.clear();
    m_weight.clear();

    // set some values from the registry
    auto set_dichotomous = [](auto& value, const TCHAR* key_name, const TCHAR* true_text, auto true_value, auto false_value)
    {
        CString setting = AfxGetApp()->GetProfileString(L"Settings", key_name, nullptr);

        if( !setting.IsEmpty() )
            value = ( setting.CompareNoCase(true_text) == 0 ) ? true_value : false_value;
    };

    set_dichotomous(m_itemSerialization, L"SaveIncluded", L"Yes", ItemSerialization::Included, ItemSerialization::Excluded);
    set_dichotomous(m_bUseVset, L"TypeValueSet", L"Yes", true, false);
    set_dichotomous(m_bHasFreqStats, L"GenerateStats", L"Yes", true, false);
    set_dichotomous(m_sortOrderAscending, L"SortOrder", L"Ascending", true, false);
    m_sortType = SortTypeFromText(TC::ToUtf8(AfxGetApp()->GetProfileString(L"Settings", L"SortType", nullptr)));
    m_outputFormat = OutputFormatFromText(TC::ToUtf8(AfxGetApp()->GetProfileString(L"Settings", L"OutputFormat", nullptr)));
}



/////////////////////////////////////////////////////////////////////////////////
//
//  int CSFreqDoc::GetPositionInList(CIMSAString name, int occurrence)
//
/////////////////////////////////////////////////////////////////////////////////
int CSFreqDoc::GetPositionInList(const wstring_view name_sv, const int occurrence, const bool reverse_search/* = false*/)
{
    auto check = [&](size_t i)
    {
        return ( SO::EqualsNoCase(name_sv, m_freqnames[i].freqnames) && occurrence == m_freqnames[i].occ );
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
        m_dictionary->LookupName(UTF8_TODO::GetUtf8(m_freqnames[i].freqnames), nullptr, nullptr, &dict_item, &dict_value_set);
        ASSERT(dict_item != nullptr);

        if( m_freqnames[i].occ < 0 && ( ( dict_item->GetOccurs() > 1 ) ||
                                        ( dict_item->GetParentItem() != nullptr && dict_item->GetParentItem()->GetOccurs() > 1 ) ) )
        {
            m_freqnames[i].occ = 0;
            ret = false;
        }

        if( dict_value_set == nullptr && dict_item->HasValueSets() )
        {
            m_freqnames[i].freqnames = UTF8_TODO::GetCString(dict_item->GetValueSet(0).GetName());
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
CString CSFreqDoc::GetNameat(const int level, const int record, const int item, const int vset, const int occ)
{
    ASSERT (level >= 0);
//  ASSERT (record >= 0);
    //if (occ >0) item = item - occ+1;
    ASSERT (item >= 0);
    ASSERT (vset >= 0);

    const CDictItem* const dict_item = m_dictionary->GetLevel(level).GetRecord(( record == -1 ) ? COMMON : record)->GetItem(item);

    return UTF8_TODO::GetCString(dict_item->HasValueSets() ? dict_item->GetValueSet(vset).GetName() :
                                                             dict_item->GetName());
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSFreqDoc::IsChecked(int position)
//
/////////////////////////////////////////////////////////////////////////////////
bool CSFreqDoc::IsChecked(const int position) const
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
    CString sOrderFile = UTF8_TODO::GetCString(Path::AppendExtension(m_baseFilePath, FileExtensions::Order));
    DeleteFile(sOrderFile);

    batchApp.AddForm(UTF8_TODO::GetUtf8(sOrderFile));

    ASSERT(m_dictionarySource.IsDefined());
    std::optional<std::string> file_based_dictionary_file_path;

    try
    {
        file_based_dictionary_file_path = m_dictionarySource.GetFileBasedDictionaryFilePath();
    }
    catch(...) { return false; }

    batchApp.AddDictionaryDescription(DictionaryDescription(*file_based_dictionary_file_path, UTF8_TODO::GetUtf8(sOrderFile), DictionaryType::Input));


    //Create the .ord file and save it
    //Create the form if the formfile does not exist
    if(!PortableFunctions::FileIsRegular(sOrderFile)) {
        CDEFormFile Order(sOrderFile, UTF8_TODO::GetCString(*file_based_dictionary_file_path));
        Order.CreateOrderFile(*m_dictionary, true);
        Order.Save(sOrderFile);
    }

    CString sFullFileName = UTF8_TODO::GetCString(Path::AppendExtension(m_baseFilePath, FileExtensions::BatchApplication));
    batchApp.SetLabel(Path::GetFilenameWithoutExtension(UTF8_TODO::GetUtf8(sFullFileName)));

    CString sAppFile = UTF8_TODO::GetCString(Path::AppendExtension(m_baseFilePath, FileExtensions::Logic));
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
bool  CSFreqDoc::CompileApp(const XTABSTMENT_TYPE eType/* = XTABSTMENT_ALL*/)
{
    if(!IsAtLeastOneItemSelected()){
        AfxMessageBox(L"You must select at least one item to tabulate\nbefore you set and compile a universe");
        return false;
    }

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CFrqOptionsView* pOptionsView = pFrame->GetFrqOptionsView();
    pOptionsView->ToDoc();

    RemoveInvalidFrequencyEntries();

    std::string old_universe;
    std::string old_weight;

    switch(eType)
    {
        case XTABSTMENT_WGHT_ONLY:
            old_universe = m_universe;
            m_universe.clear();
            break;

        case XTABSTMENT_UNIV_ONLY:
            old_weight = m_weight;
            m_weight.clear();
            break;

        case XTABSTMENT_ALL:
        default:
            break;
    }

    if(!GenerateBchForFrq()){
        AfxMessageBox(L"Failed to generate freq app");
        if(!old_universe.empty()){
            m_universe = old_universe;
        }
        if(!old_weight.empty()){
            m_weight = old_weight;
        }
        return false;
    }
    else {
        if(!old_universe.empty()){
            m_universe = old_universe;
        }
        if(!old_weight.empty()){
            m_weight = old_weight;
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
void CSFreqDoc::WriteDefaultFiles(Application* pApplication, const CString& sAppFName)
{
    //AppFile
    CString sAppSCodeFName(sAppFName);
    PathRemoveExtension(sAppSCodeFName.GetBuffer(_MAX_PATH));
    sAppSCodeFName.ReleaseBuffer();
    sAppSCodeFName += L"." + UTF8_TODO::GetCString(FileExtensions::Logic);

    CFileStatus fStatus;
    BOOL bRet = CFile::GetStatus(sAppSCodeFName,fStatus);
    if(!bRet){
        //Create the .app file
        CSpecFile appFile(TRUE);
        appFile.Open(sAppSCodeFName,CFile::modeWrite);
        appFile.WriteString(UTF8_TODO::GetCString(m_logicSettings.GetDefaultFirstLineForTextSource(pApplication->GetLabel(), AppFileType::Code)));
        appFile.Close();
    }

    pApplication->AddCodeFile(CodeFile(CodeType::LogicMain, std::make_unique<TextSource>(UTF8_TODO::GetUtf8(sAppSCodeFName))));
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
            std::unique_ptr<std::tuple<std::string, std::string>>& universe_and_weight = *reinterpret_cast<std::unique_ptr<std::tuple<std::string, std::string>>*>(wParam);
            ASSERT(universe_and_weight == nullptr);

            universe_and_weight = std::make_unique<std::tuple<std::string, std::string>>(m_document->m_universe,
                                                                                         m_document->m_weight);

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
        batch_executor.AddUWMCallback(UWM::Freq::GetUniverseAndWeight, std::make_unique<GetUniverseAndWeightCallback>(this));
        batch_executor.Run(UTF8_TODO::GetUtf8(m_batchPff->GetPifFileName()));
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


std::string CSFreqDoc::GenerateFrqCmd()
{
    CWaitCursor wait;
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    CFrqOptionsView* const pOptionsView = pFrame->GetFrqOptionsView();
    pOptionsView->ToDoc();

    CSFreqView* const pFrqTreeView = pFrame->GetFreqTreeView();
    CFreqDDTreeCtrl* const pDictTree = pFrqTreeView->GetDictTree();

    // generate the optional commands
    std::vector<std::string> extra_commands;

    SO::MakeTrim(m_universe);

    if( !m_universe.empty() )
        extra_commands.emplace_back(FormatText("universe(%s)", m_universe.c_str()));

    SO::MakeTrim(m_weight);

    if( !m_weight.empty() )
    {
        extra_commands.emplace_back(FormatText("weight(%s)", m_weight.c_str()));

        // if the weight is a constant number with decimals, apply the decimal setting
        if( StringToNumber(m_weight) != DEFAULT )
        {
            const size_t decimal_pos = m_weight.find('.');

            if( decimal_pos != std::string::npos )
            {
                const int number_decimals = m_weight.length() - decimal_pos - 1;
                extra_commands.emplace_back(FormatText("decimals(%d)", std::min(number_decimals, 5)));
            }
        }
    }

    if( m_bHasFreqStats )
    {
        extra_commands.emplace_back("stat");

        if( m_percentiles.has_value() )
            extra_commands.emplace_back(FormatText("percentiles(%d)", *m_percentiles));
    }

    if( !m_sortOrderAscending || m_sortType != DefaultSortType )
    {
        std::string& sort_command = extra_commands.emplace_back("sort(");

        if( !m_sortOrderAscending )
            sort_command.append("descending ");

        sort_command.append("by ")
                    .append(SO::ToLower(SortTypeNames[static_cast<size_t>(m_sortType)]))
                    .push_back(')');
    }

     extra_commands.emplace_back("nonetpercents");

     if( !m_bUseVset )
         extra_commands.emplace_back("distinct");


    // see what frequencies must be generated
    std::vector<std::vector<SelectedFrequencies>> grouped_selected_frequencies =
        SelectedFrequenciesWorker(*m_dictionary, pDictTree).GenerateByFrequencyGroup();
    ASSERT(!grouped_selected_frequencies.empty());

    std::string freq_command = "PROC GLOBAL\n";
    const DictLevel* last_level_added = nullptr;
    const CDictRecord* last_record_added = nullptr;
    const char* Tabs[] = { "\t", "\t\t", "\t\t\t", "\t\t\t\t" };
    const char** current_tabs_index = &Tabs[0];

    auto end_record_for_loop = [&]
    {
        if( last_record_added != nullptr && last_record_added->GetMaxRecs() > 1 )
            freq_command.append("\n\tendfor;\n");

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

            freq_command.append(FormatText("\n\nPROC %s\n", first_selected_frequency_in_group.level->GetName().c_str()));
            last_level_added = first_selected_frequency_in_group.level;
        }


        // add a for loop for the record if necessary
        if( first_selected_frequency_in_group.record != last_record_added )
        {
            end_record_for_loop();

            if( first_selected_frequency_in_group.record->GetMaxRecs() > 1 )
            {
                freq_command.append(FormatText("\n\tfor numeric csfreq_record_occurrence in %s_EDT do\n",
                                               first_selected_frequency_in_group.record->GetName().c_str()));
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
            freq_command.append(FormatText("\n%sFreq\n%sinclude(", *current_tabs_index, *current_tabs_index));

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
                    if( last_item_added != nullptr )
                        freq_command.append(", ");

                    freq_command.append(selected_frequency.item->GetName());

                    if( selected_frequency.occurrence.has_value() )
                    {
                        freq_command.append(FormatText("(%s%u)", ( selected_frequency.record->GetMaxRecs() > 1 ) ? "*, " : "",
                                                                 *selected_frequency.occurrence + 1));
                    }

                    last_item_added = selected_frequency.item;
                }
            }

            freq_command.append(")\n");


            // add any value sets
            if( !specified_value_sets.empty() )
            {
                freq_command.append(*current_tabs_index)
                            .append("valueset(");

                const DictValueSet* last_value_set_added = nullptr;

                for( const DictValueSet* value_set : specified_value_sets )
                {
                    ASSERT(last_value_set_added != value_set);

                    if( last_value_set_added != nullptr )
                        freq_command.append(", ");

                    freq_command.append(value_set->GetName());

                    last_value_set_added = value_set;
                }

                freq_command.append(")\n");
            }


            // add the the extra commands and end the freq command
            for( const std::string& extra_command : extra_commands )
            {
                freq_command.append(*current_tabs_index)
                            .append(extra_command)
                            .append("\n");
            }

            freq_command.append(*current_tabs_index)
                        .append(";\n");
        };



        auto freq_itr = grouped_selected_frequency.cbegin();
        auto freq_itr_end = grouped_selected_frequency.cend();

        if( !first_selected_frequency_in_group.occurrence.has_value() )
        {
            add_items(freq_itr, freq_itr_end);
        }

        // add an item for loop if necessary
        else
        {
            freq_command.append(FormatText("\n%sfor numeric csfreq_item_occurrence in %s000 do\n", *current_tabs_index,
                                           GetParentItemIfRepeating(first_selected_frequency_in_group.item)->GetName().c_str()));
            ++current_tabs_index;

            while( freq_itr != freq_itr_end )
            {
                auto freq_itr_start = freq_itr;

                do
                {
                    ++freq_itr;

                } while( freq_itr != freq_itr_end && freq_itr->occurrence == freq_itr_start->occurrence );

                freq_command.append(FormatText("\n%sif csfreq_item_occurrence = %u then\n", *current_tabs_index,
                                                                                            *freq_itr_start->occurrence + 1));
                ++current_tabs_index;

                add_items(freq_itr_start, freq_itr);

                --current_tabs_index;
                freq_command.append(FormatText("\n%sendif;\n", *current_tabs_index));
            }

            --current_tabs_index;
            freq_command.append(FormatText("\n%sendfor;\n", *current_tabs_index));
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
    pCmdUI->Enable(GetTitle() != L"Untitled");
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSFreqDoc::OnFileSaveAs()
//
/////////////////////////////////////////////////////////////////////////////////

void CSFreqDoc::OnFileSaveAs()
{
    std::string file_path = UTF8_TODO::GetUtf8(m_FreqPiff.GetAppFName());         // BMD 14 Mar 2002

    // if no spec file path exists, base it on the dictionary's source path
    if( file_path.empty() )
        file_path = Path::ReplaceExtension(m_dictionarySource.GetSourceFilePath(), FileExtensions::FrequencySpec);

    SaveFileDlg save_file_dlg(0, FileExtensions::FrequencySpec, file_path, L"Frequency Specification Files (*.fqf)|*.fqf|All Files (*.*)|*.*||");
    save_file_dlg.SetTitle(L"Save Frequency Specification File");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    m_FreqPiff.SetAppFName(UTF8_TODO::GetCString(save_file_dlg.GetFilePath()));
    SaveSpecFile();
    SetModifiedFlag(FALSE);
    AfxGetApp()->AddToRecentFileList(TC::ToWide(save_file_dlg.GetFilePath()).c_str());
    SetPathName(m_FreqPiff.GetAppFName(), TRUE);
    m_FreqPiff.SetPifFileName(UTF8_TODO::GetCString(Path::AppendExtension(save_file_dlg.GetFilePath(), FileExtensions::Pff)));
    m_FreqPiff.Save();     // BMD 14 Mar 2002

    m_bSaved = true;
}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CSFreqDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CSFreqDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetTitle() != L"Untitled");
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
    m_bSaved = false;

    if (m_FreqPiff.GetAppFName().IsEmpty()) {
        OnFileSaveAs();
    }
    else {
        SaveSpecFile();
        SetModifiedFlag(FALSE);
        m_bSaved = true;
    }
}


void CSFreqDoc::DoPostRunCleanUp()
{
#ifndef _DEBUG
    //delete the .pff and other files
    if( !m_batchPff->GetPifFileName().IsEmpty() )
    {
        PortableFunctions::FileDelete(m_batchPff->GetPifFileName());
        PortableFunctions::FileDelete(Path::AppendExtension(m_baseFilePath, FileExtensions::BatchApplication));
        PortableFunctions::FileDelete(Path::AppendExtension(m_baseFilePath, FileExtensions::Order));
        PortableFunctions::FileDelete(Path::AppendExtension(m_baseFilePath, FileExtensions::Logic));
        PortableFunctions::FileDelete(Path::AppendExtension(m_baseFilePath, FileExtensions::TableSpec));
    }
#endif
}


bool CSFreqDoc::ExecuteFileInfo()
{
    bool bRet = true;

    const std::string base_name_for_files = m_FreqPiff.GetAppFName().IsEmpty() ? "CSFrqRun" :
                                                                                 Path::GetFilenameWithoutExtension(UTF8_TODO::GetUtf8(m_FreqPiff.GetAppFName()));

    std::string directory_for_files = PathHelpers::GetDirectoryName({ UTF8_TODO::GetUtf8(m_FreqPiff.GetAppFName()), m_dictionarySource.GetSourceFilePath() });

    if( directory_for_files.empty() )
        directory_for_files = GetTempDirectory();

    if( m_FreqPiff.GetListingFName().IsEmpty() )
        m_FreqPiff.SetListingFName(UTF8_TODO::GetCString(PathHelpers::GetFilePathInDirectory(Path::AppendExtension(base_name_for_files, FileExtensions::Listing), directory_for_files)));

    if( m_FreqPiff.GetFrequenciesFilename().IsEmpty() )
        m_FreqPiff.SetFrequenciesFilename(UTF8_TODO::GetCString(PathHelpers::GetFilePathInDirectory(Path::AppendExtension(base_name_for_files, FileExtensions::Table), directory_for_files)));


    // make sure the output format extension matches the selection
    const char* const output_format_extension =
        ( m_outputFormat == OutputFormat::Table ) ?   FileExtensions::Table :
        ( m_outputFormat == OutputFormat::HTML )  ?   FileExtensions::HTML :
        ( m_outputFormat == OutputFormat::Json )  ?   FileExtensions::Json :
        ( m_outputFormat == OutputFormat::Text )  ?   FileExtensions::Listing :
      /*( m_outputFormat == OutputFormat::Excel ) ? */FileExtensions::Excel;

    const std::string current_extension = PortableFunctions::PathGetFileExtension(UTF8_TODO::GetUtf8(m_FreqPiff.GetFrequenciesFilename()));

    if( !SO::EqualsNoCase(current_extension, output_format_extension) )
    {
        m_FreqPiff.SetFrequenciesFilename(PortableFunctions::PathRemoveFileExtensionCS(m_FreqPiff.GetFrequenciesFilename())
            + L"." + UTF8_TODO::GetCString(output_format_extension));
    }

    // make sure the frequencies filename isn't the same as the listing filename
    if( m_FreqPiff.GetFrequenciesFilename().CompareNoCase(m_FreqPiff.GetListingFName()) == 0 )
    {
        m_FreqPiff.SetFrequenciesFilename(PortableFunctions::PathRemoveFileExtensionCS(m_FreqPiff.GetFrequenciesFilename())
            + L".freq." + UTF8_TODO::GetCString(output_format_extension));
    }


    if( !m_batchmode )
    {
        // don't ask for a data file if a data source with an embedded dictionary was opened
        if( m_dictionarySource.UsingEmbeddedDictionary() )
        {
            m_FreqPiff.SetSingleInputDataConnectionString(m_dictionarySource.GetConnectionString());
        }

        else
        {
            DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true, m_FreqPiff.GetInputDataConnectionStringsSerializable());
            data_file_dlg.SetTitle(L"Select Data File(s) to Tabulate")
                         .SetDictionaryFilePath(m_dictionarySource.GetDictionaryFilePath())
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
    AfxGetApp()->WriteProfileString(L"Settings", L"SaveIncluded", GetSaveExcludedItems() ? L"No" : L"Yes");
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
    BatchLogicViewerDlg dlg(*m_dictionary, m_logicSettings, GenerateFrqCmd());
    dlg.DoModal();
}


void CSFreqDoc::ProcessDictionarySource(DictionarySource dictionary_source)
{
    m_dictionarySource = std::move(dictionary_source);

    try
    {
        m_dictionary = m_dictionarySource.GetDictionary();
    }

    catch(...)
    {
        m_dictionarySource.Reset();
        m_dictionary = std::make_unique<CDataDict>();
        throw;
    }
}


std::string CSFreqDoc::GetDocumentWindowTitle() const
{
    std::string document_title = !m_FreqPiff.GetAppFName().IsEmpty() ? Path::GetFilename(UTF8_TODO::GetUtf8(m_FreqPiff.GetAppFName())) :
                                 m_dictionarySource.IsDefined()      ? m_dictionarySource.GetConnectionString().ToDisplayString(true) :
                                                                       std::string();

    // add the dictionary name when possible
    if( !document_title.empty() && m_dictionary != nullptr )
        return SO::CreateParentheticalExpression(std::move(document_title), m_dictionary->GetName());

    return document_title;
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
    m_batchPff->SetPifFileName(UTF8_TODO::GetCString(Path::AppendExtension(m_baseFilePath, FileExtensions::Pff)));
    m_batchPff->SetAppType(BATCH_TYPE);
    m_batchPff->SetAppFName(UTF8_TODO::GetCString(Path::AppendExtension(m_baseFilePath, FileExtensions::BatchApplication)));

    if (m_batchmode && !m_FreqPiff.GetStartLanguageString().IsEmpty())
        m_batchPff->SetStartLanguageString(m_FreqPiff.GetStartLanguageString());
    else
        m_batchPff->SetStartLanguageString(UTF8_TODO::GetCString(m_dictionary->GetCurrentLanguage().GetName()));

    // OnExit should only be executed if CSFreq was run with a PFF as a command line argument
    if( !m_batchmode )
        m_batchPff->SetOnExitFilename(CString());
}



/////////////////////////////////////////////////////////////////////////////////
//
// Spec file serialization
//
/////////////////////////////////////////////////////////////////////////////////

CREATE_JSON_VALUE(frequencies)

CREATE_ENUM_JSON_SERIALIZER(ItemSerialization,
    { ItemSerialization::Included, "included" },
    { ItemSerialization::Excluded, "excluded" })

CREATE_ENUM_JSON_SERIALIZER(OutputFormat,
    { OutputFormat::Table, "TBW" },
    { OutputFormat::HTML,  "HTML" },
    { OutputFormat::Json,  "JSON" },
    { OutputFormat::Text,  "text" },
    { OutputFormat::Excel, "Excel" })


bool CSFreqDoc::OpenSpecFile(const std::string& spec_file_path, const bool silent)
{
    try
    {
        const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(spec_file_path, nullptr, [&]() { return ConvertPre80SpecFile(spec_file_path); });

        try
        {
            json_reader->CheckVersion();
            json_reader->CheckFileType(JV::frequencies);

            // open the dictionary
            ProcessDictionarySource(json_reader->Get<DictionarySource>(JK::dictionary));

            // reestablish the dictionary language
            const std::optional<std::string_view> language_name_sv = json_reader->GetOptional<std::string_view>(JK::language);

            if( language_name_sv.has_value() )
            {
                const std::optional<size_t> language_index = m_dictionary->IsLanguageDefined(*language_name_sv);

                if( language_index.has_value() )
                {
                    m_dictionary->SetCurrentLanguage(*language_index);
                }

                else
                {
                    json_reader->LogWarning("The dictionary language '%s' is not in the dictionary '%s'",
                                            std::string(*language_name_sv).c_str(), m_dictionary->GetName().c_str());
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
                    json_reader->LogWarning("The percentiles value '%d' is not valid and been reset", *m_percentiles);
                    m_percentiles.reset();
                }
            }

            const auto& sort_node = json_reader->GetOrEmpty(JK::sort);
            m_sortOrderAscending = sort_node.GetOrDefault(JK::ascending, true);
            m_sortType = sort_node.GetOrDefault(JK::order, DefaultSortType);

            m_logicSettings = json_reader->GetOrDefault(JK::logicSettings, m_logicSettings);

            m_universe = json_reader->GetOrConstruct<std::string>(JK::universe);
            m_weight = json_reader->GetOrConstruct<std::string>(JK::weight);

            m_itemSerialization = json_reader->GetOrDefault(JK::itemSerialization, ItemSerialization::Included);

            // create the list of all the items from the dictionary
            AddAllItems();

            // read the items
            for( const auto& item_node : json_reader->GetArrayOrEmpty(JK::items) )
            {
                const std::optional<std::string_view> item_name_sv = item_node.GetOptional<std::string_view>(JK::name);

                if( item_name_sv.has_value() )
                {
                    const int occurrence = item_node.GetOrDefault(JK::occurrence, -1);
                    const int pos = GetPositionInList(UTF8_TODO::GetWide(*item_name_sv), occurrence);

                    if( pos == -1 )
                    {
                        json_reader->LogWarning("'%s' is not a valid item or value set in the dictionary '%s'",
                                                std::string(*item_name_sv).c_str(), m_dictionary->GetName().c_str());
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
            json_reader->GetMessageLogger().RethrowException(spec_file_path, exception);
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
        const std::unique_ptr<JsonFileWriter> json_writer = JsonSpecFile::CreateWriter(m_FreqPiff.GetAppFName(), JV::frequencies);

        json_writer->Write(JK::dictionary, m_dictionarySource)
                    .Write(JK::language, m_dictionary->GetCurrentLanguage().GetName())
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

        json_writer->WriteIfNotBlank(JK::universe, m_universe)
                    .WriteIfNotBlank(JK::weight, m_weight);

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


std::string CSFreqDoc::ConvertPre80SpecFile(const InterfaceString file_path)
{
    CSpecFile specfile;

    if( !specfile.Open(file_path.GetString<std::wstring>().c_str(), CFile::modeRead) )
        throw CSProException("Failed to open the Tabulate Frequencies specification file: %s", file_path.c_str_utf8());

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::version, 7.7);
    json_writer->Write(JK::fileType, JV::frequencies);

    json_writer->Write(JK::logicSettings, LogicSettings::GetOriginalSettings());

    try
    {
        // Is correct spec file?
        if( !specfile.IsHeaderOK(L"[CSFreq]") )
            throw CSProException("Spec File does not begin with\n\n    [CSFreq]");

        // Ignore version errors
        specfile.IsVersionOK(Versioning::CSProVersionText);

        CString command;
        CString argument;

        bool sort_ascending = true;
        FrequencyPrinterOptions::SortType sort_type = DefaultSortType;

        std::vector<std::tuple<CString, int>> names_and_occurrences;

        while( specfile.GetLine(command, argument) == SF_OK )
        {
            if( command.CompareNoCase(L"File") == 0 )
            {
                json_writer->Write(JK::dictionary, specfile.EvaluateRelativeFilename(argument));
            }

            else if( command.CompareNoCase(L"ItemsAre") == 0 )
            {
                json_writer->Write(JK::itemSerialization, SO::ToLower(wstring_view(argument)));
            }

            else if( command.CompareNoCase(L"UseVSet") == 0 )
            {
                json_writer->Write(JK::useValueSets, ( argument.CompareNoCase(L"Yes") == 0 ));
            }

            else if( command.CompareNoCase(L"GenerateStats") == 0 )
            {
                json_writer->Write(JK::statistics, ( argument.CompareNoCase(L"Yes") == 0 ));
            }

            else if( command.CompareNoCase(L"Percentiles") == 0 )
            {
                json_writer->Write(JK::percentiles, _ttoi(argument));
            }

            else if( command.CompareNoCase(L"SortOrder") == 0 )
            {
                sort_ascending = ( argument.CompareNoCase(L"Descending") != 0 );
            }

            else if( command.CompareNoCase(L"SortType") == 0 )
            {
                sort_type = SortTypeFromText(UTF8_TODO::GetUtf8(argument));
            }

            else if( command.CompareNoCase(L"OutputFormat") == 0 )
            {
                json_writer->Write(JK::output, OutputFormatFromText(UTF8_TODO::GetUtf8(argument)));
            }

            else if( command.CompareNoCase(L"Universe") == 0 )
            {
                json_writer->Write(JK::universe, argument);
            }

            else if( command.CompareNoCase(L"Weight") == 0 )
            {
                json_writer->Write(JK::weight, argument);
            }

            else if( command.CompareNoCase(L"Language") == 0 )
            {
                json_writer->Write(JK::language, argument);
            }

            else if( command.CompareNoCase(L"[Item]") == 0 )
            {
                names_and_occurrences.emplace_back(CString(), -1);
            }

            else if( command.CompareNoCase(L"Name") == 0 && !names_and_occurrences.empty() )
            {
                std::get<0>(names_and_occurrences.back()) = argument;
            }

            else if( command.CompareNoCase(L"Occ") == 0 && !names_and_occurrences.empty() )
            {
                if( !argument.IsEmpty() )
                    std::get<1>(names_and_occurrences.back()) = _ttoi(argument);
            }

            else if( command.CompareNoCase(L"[Dictionaries]") != 0 &&
                     command.CompareNoCase(L"[Items]") != 0 &&
                     command.CompareNoCase(L"[Item]") != 0 &&
                     command.CompareNoCase(L"[EndItem]") != 0 &&
                     command.CompareNoCase(L"Stats") != 0 &&
                     command.CompareNoCase(L"NTiles") != 0 )
            {
                throw CSProException("Spec File: Invalid command: %s", UTF8_TODO::GetUtf8(command).c_str());
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

        throw CSProException("There was an error reading the Tabulate Frequencies specification file %s:\n\n%s",
                             PortableFunctions::PathGetFilename(file_path.GetString<std::string>()).c_str(), exception.what());
    }

    json_writer->EndObject();

    return json_writer->ReleaseString();
}
