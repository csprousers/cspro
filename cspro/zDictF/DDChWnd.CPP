//***************************************************************************
//  File name: DDChWnd.cpp
//
//  Description:
//       Data Dictionary child window implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "DDChWnd.H"
#include "DDLView.h"
#include "FindDlg.h"
#include <zUtilF/TextReportDlg.h>
#include <zDictO/ValueSetResponse.h>


namespace DictFind
{
    CFindDlg dlg;
    bool active = false;
}


namespace
{
    void UpdateRelativePositionsStatusBar(const CDataDict& dictionary)
    {
        const TCHAR* text = dictionary.IsPosRelative() ? _T("Relative Positions") : _T("Absolute Positions");
        AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, (WPARAM)text);
    }
}



/////////////////////////////////////////////////////////////////////////////
// CDictChildWnd

IMPLEMENT_DYNCREATE(CDictChildWnd, COXMDIChildWndSizeDock)

CDictChildWnd::CDictChildWnd()
    :   m_bLayout(FALSE), m_bQuestionnaireView(FALSE), m_iGridViewPaneSize(0),
        m_bViewPropertiesPanel(FALSE), m_pQuestionnaireView(nullptr)
{
    // Get Global Settings
    TCHAR pszTemp[10];
    GetPrivateProfileString(_T("intl"), _T("sDecimal"), _T("."), pszTemp, 10, _T("WIN.INI"));
    m_cDecimal = pszTemp[0];
}

CDictChildWnd::~CDictChildWnd()
{
}


BEGIN_MESSAGE_MAP(CDictChildWnd, COXMDIChildWndSizeDock)
    //{{AFX_MSG_MAP(CDictChildWnd)
    ON_WM_MDIACTIVATE()
    ON_WM_SYSCOMMAND()
    ON_COMMAND(ID_VIEW_LAYOUT, OnViewLayout)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, OnUpdateEditFind)
    ON_COMMAND(ID_EDIT_FIND, OnEditFind)
    ON_COMMAND(ID_FILE_PAGE_SETUP, OnFilePageSetup)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LAYOUT, OnUpdateViewLayout)
    ON_COMMAND(ID_VIEW_ALIASES, OnViewAliases)
    ON_COMMAND(ID_DICT_ANALYSIS_ITEMS_NO_VALUE_SETS, OnDictAnalysisItemsNoValueSets)
    ON_COMMAND(ID_DICT_ANALYSIS_NUMERIC_ITEMS_NO_VALUE_SETS, OnDictAnalysisNumericItemsNoValueSets)
    ON_COMMAND(ID_DICT_ANALYSIS_NUMERIC_ITEMS_OVERLAPPING_VALUE_SETS, OnDictAnalysisNumericItemsOverlappingValueSets)
    ON_COMMAND(ID_DICT_ANALYSIS_NUMERIC_ITEMS_MISMATCHED_ZF_DC, OnDictAnalysisNumericMismatchedZeroFillDecChar)
    ON_MESSAGE(UWM::Dictionary::Find, OnFind)        // Find text
    ON_COMMAND(ID_VIEW_PROPERTIES, &CDictChildWnd::OnViewProperties)
    ON_UPDATE_COMMAND_UI(ID_VIEW_PROPERTIES, &CDictChildWnd::OnUpdateViewProperties)
    ON_COMMAND(ID_VIEW_QUESTIONNAIRE, &CDictChildWnd::OnViewQuestionnaire)
    ON_UPDATE_COMMAND_UI(ID_VIEW_QUESTIONNAIRE, &CDictChildWnd::OnUpdateViewQuestionnaire)
    ON_UPDATE_COMMAND_UI(ID_EDIT_LANGUAGES, &CDictChildWnd::OnUpdateEditLanguages)
    ON_UPDATE_COMMAND_UI(ID_EDIT_RELATION, &CDictChildWnd::OnUpdateEditRelation)
    ON_UPDATE_COMMAND_UI(ID_EDIT_SECURITY_OPTIONS, &CDictChildWnd::OnUpdateEditSecurityOptions)
    ON_UPDATE_COMMAND_UI(ID_DICTIONARY_MACROS, &CDictChildWnd::OnUpdateDictionaryMacros)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_ENABLE_BINARY_ITEMS, &CDictChildWnd::OnUpdateOptionsEnableBinaryItems)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_ABS_REL, &CDictChildWnd::OnUpdateOptionsAbsRel)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_ZEROFILL, &CDictChildWnd::OnUpdateOptionsZeroFill)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_DECCHAR, OnUpdateOptionsDecChar)
    //}}AFX_MSG_MAP


    ON_COMMAND(ID_VIEW_DICTIONARY, &CDictChildWnd::OnViewDictionary)
    ON_UPDATE_COMMAND_UI(ID_VIEW_DICTIONARY, &CDictChildWnd::OnUpdateViewDictionary)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDictChildWnd message handlers

BOOL CDictChildWnd::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Add your specialized code here and/or call the base class
//  cs.style &= ~(WS_SYSMENU);
    cs.style |= WS_MAXIMIZE;
    return CMDIChildWnd::PreCreateWindow(cs);
}

void CDictChildWnd::ActivateFrame(int nCmdShow) {

    nCmdShow = SW_SHOWMAXIMIZED;                        // See PreCreateWindow above
    CMDIChildWnd::ActivateFrame(nCmdShow);
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
    if( pDoc ) {
        pDoc->IsDocOK();
    }

    if (GetDictDlgBar()->GetSafeHwnd() && m_bViewPropertiesPanel) {
        GetDictDlgBar()->Invalidate(FALSE);
    }

    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, static_cast<WPARAM>(FrameType::Dictionary));
    UpdateRelativePositionsStatusBar(*pDoc->GetDict());
}

void CDictChildWnd::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
    CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
    CDDDoc* pDoc1 = assert_cast<CDDDoc*>(GetActiveDocument());
    POSITION pos = pDoc1->GetFirstViewPosition();
    ASSERT(pos != NULL);
    CDDGView* pView = (CDDGView*) pDoc1->GetNextView(pos);
    if (bActivate && pActivateWnd && pActivateWnd->IsKindOf(RUNTIME_CLASS(CDictChildWnd))) {
        if (pDoc1 && pDoc1->GetDictTreeCtrl()) {
            pDoc1->GetDictTreeCtrl()->ResetTreeIcons();
        }
        DictFind::dlg.SetCurrView(pView);
        if (DictFind::dlg.GetSafeHwnd() != NULL) {
            DictFind::dlg.ShowWindow(SW_SHOW);
        }

        if (GetDictDlgBar()->GetSafeHwnd() && m_bViewPropertiesPanel) {
            GetDictDlgBar()->Invalidate(FALSE);
        }
        AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, static_cast<WPARAM>(FrameType::Dictionary));
        AfxGetMainWnd()->SendMessage(UWM::Designer::SelectTab, static_cast<WPARAM>(FrameType::Dictionary));
    }
    if(!bActivate && pDeactivateWnd && pDeactivateWnd->IsKindOf(RUNTIME_CLASS(CDictChildWnd))) {
        if (DictFind::dlg.GetSafeHwnd() != NULL) {
            DictFind::dlg.ShowWindow(SW_HIDE);
        }
        CDocument* pDoc2 = GetActiveDocument();
        if(pDoc2->IsKindOf(RUNTIME_CLASS(CDDDoc))) {
            CDataDict* pDict = assert_cast<CDDDoc*>(pDoc2)->GetDict();
            pDict->UpdatePointers();
            CDDGrid& current_grid = pView->GetCurrentGrid();
            if (current_grid.IsEditing()) {
                current_grid.EditChange(VK_CANCEL);
            }
        }

        if(pActivateWnd == nullptr || !pActivateWnd->IsKindOf(RUNTIME_CLASS(CDictChildWnd))) {
            AfxGetMainWnd()->SendMessage(UWM::Designer::HideToolbar);
        }
    }

    if (bActivate) {
        UpdateRelativePositionsStatusBar(*pDoc1->GetDict());
    }
}



BOOL CDictChildWnd::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CMDIFrameWnd* pParentWnd, CCreateContext* pContext) {

    dwStyle |= WS_MAXIMIZE;
    BOOL bRet =  CMDIChildWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, pContext);
    if(bRet) {
        // Get the window's menu
        CMenu* pMenu = GetSystemMenu(FALSE);
        VERIFY(pMenu->GetSafeHmenu());

        // Disable the 'X'
        VERIFY(::DeleteMenu(pMenu->GetSafeHmenu(), SC_CLOSE , MF_BYCOMMAND));
        VERIFY(::DeleteMenu(pMenu->GetSafeHmenu(), 5 , MF_BYPOSITION));

        // Force immediate menu update
        DrawMenuBar();

        EnableDocking(CBRS_ALIGN_ANY);

    }
    return bRet;
}

void CDictChildWnd::OnSysCommand( UINT nID, LPARAM lParam )
{
    if(nID == SC_CLOSE)  {
        return;
    }
    else {
        CMDIChildWnd::OnSysCommand(nID,lParam);
    }
}

BOOL CDictChildWnd::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext) {

    // Create splitter window
    CRect rect;
    AfxGetMainWnd()->GetClientRect(&rect);
    int height = rect.Height();
    if (!m_wndSplitter.CreateStatic(this, 3, 1)) {
        return FALSE;
    }
    if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CDDGView), CSize(100, height), pContext) ||
            !m_wndSplitter.CreateView(1, 0, RUNTIME_CLASS(CDDLView), CSize(100, 0), pContext) ||
            !m_wndSplitter.CreateView(2, 0, RUNTIME_CLASS(CDDLView), CSize(100, 0), pContext)) {
        m_wndSplitter.DestroyWindow();
        return FALSE;
    }
    m_wndSplitter.SetRowInfo(2, 0, 0);
    return TRUE;
    // TODO: Add your specialized code here and/or call the base class
//  return CMDIChildWnd::OnCreateClient(lpcs, pContext);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDictChildWnd::OnUpdateViewLayout
//
/////////////////////////////////////////////////////////////////////////////

void CDictChildWnd::OnUpdateViewLayout(CCmdUI* pCmdUI) {
    pCmdUI->SetCheck(m_bLayout);
    m_bQuestionnaireView ? pCmdUI->Enable(FALSE) : pCmdUI->Enable(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDictChildWnd::OnViewLayout
//
/////////////////////////////////////////////////////////////////////////////

void CDictChildWnd::OnViewLayout() {
    m_bQuestionnaireView = FALSE;
    CRect rect;
    AfxGetMainWnd()->GetClientRect(&rect);
    int height = rect.Height();
    m_wndSplitter.GetPane(0, 0)->ShowWindow(SW_SHOW);
    m_wndSplitter.GetPane(1, 0)->ShowWindow(SW_SHOW);
    m_wndSplitter.GetPane(2, 0)->ShowWindow(SW_HIDE);
    if (m_bLayout) {
        m_bLayout = FALSE;
        m_wndSplitter.SetRowInfo(0, height, 0);
        m_wndSplitter.SetRowInfo(1, 0, 0);
        m_wndSplitter.SetRowInfo(2, 0, 0);
    }
    else {
        m_bLayout = TRUE;
        m_wndSplitter.SetRowInfo(0, 2*height/3, 0);
        m_wndSplitter.SetRowInfo(1, height/3, 0);
        m_wndSplitter.SetRowInfo(2, 0, 0);
    }
    m_wndSplitter.SetSplitterBarSizes();
    m_wndSplitter.RecalcLayout();
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDictChildWnd::OnUpdateEditFind
//
/////////////////////////////////////////////////////////////////////////////

void CDictChildWnd::OnUpdateEditFind(CCmdUI* pCmdUI) {
    if (m_bQuestionnaireView) {
        pCmdUI->Enable(FALSE);
    }
    else {
        pCmdUI->Enable(TRUE);
        pCmdUI->SetCheck(DictFind::active);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDictChildWnd::OnEditFind
//
/////////////////////////////////////////////////////////////////////////////

void CDictChildWnd::OnEditFind() {

    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
    if (pDoc->IsPrintPreview()) {
        return;
    }
    if (DictFind::active) {
        DictFind::dlg.SetFocus();    // BMD 10 Jul 2003
    }
    else {
        DictFind::active = true;
        if (DictFind::dlg.GetSafeHwnd() == NULL) {
            // first time through, need to create the dialog ...
            CRect rcViewClient, rcDlgRect;
            CPoint ptDlgOrigin;
            DictFind::dlg.Create();
            DictFind::dlg.GetWindowRect(&rcDlgRect);   // position of the dialog, relative to client view
            m_wndSplitter.GetPane(0,0)->GetClientRect(&rcViewClient);                         // right,bottom hold size of client view
            ptDlgOrigin = CPoint(rcViewClient.right-rcDlgRect.Size().cx, rcViewClient.bottom-rcDlgRect.Size().cy);    // left, top of dialog, relative to client view
            ClientToScreen(&ptDlgOrigin );                      // convert to absolute (screen) coordinates
            DictFind::dlg.MoveWindow(ptDlgOrigin.x - 20, ptDlgOrigin.y, rcDlgRect.Size().cx, rcDlgRect.Size().cy);  // x,y,width,height
            DictFind::dlg.SetWindowText(_T("Find Dictionary Text"));
            DictFind::dlg.ShowWindow (SW_SHOW);
        }
        // Set the input focus to the "Find What" combo box ...
        DictFind::dlg.GotoDlgCtrl ((CComboBox*) DictFind::dlg.GetDlgItem(IDC_SEARCH_TEXT));
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDictChildWnd::OnFind
//
/////////////////////////////////////////////////////////////////////////////

void CDictChildWnd::SetFindActive(bool bFind)
{
    DictFind::active = bFind;
}

LRESULT CDictChildWnd::OnFind(WPARAM /*wParam*/, LPARAM /*lParam*/) {

    int iLevel, iRec, iItem, iVSet, iValue;
    long iRow;

    // Get Grid View
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
    POSITION pos = pDoc->GetFirstViewPosition();
    ASSERT(pos != NULL);
    CDDGView* pView = (CDDGView*) pDoc->GetNextView(pos);

    // Get current focus position
    if (pView->m_iGrid == DictionaryGrid::Dictionary) {
        if (pView->m_gridDict.IsEditing()) {
            return 0;
        }
        iRow = pView->m_gridDict.GetCurrentRow();
        if (iRow < 3) {
            iLevel = NONE;
        }
        else {
            iLevel = iRow - 3;
        }
        iRec   = NONE;
        iItem  = NONE;
        iVSet  = NONE;
        iValue = NONE;
    }
    else if (pView->m_iGrid == DictionaryGrid::Level) {
        if (pView->m_gridLevel.IsEditing()) {
            return 0;
        }
        iLevel = pView->m_gridLevel.GetLevel();
        iRec   = pView->m_gridLevel.GetCurrentRow();
        iItem  = NONE;
        iVSet  = NONE;
        iValue = NONE;
    }
    else if (pView->m_iGrid == DictionaryGrid::Record) {
        if (pView->m_gridRecord.IsEditing()) {
            return 0;
        }
        iLevel = pView->m_gridRecord.GetLevel();
        iRow   = pView->m_gridRecord.GetCurrentRow();
        if (pView->m_gridRecord.GetLevel(iRow) == NONE) {
            iRec = pView->m_gridRecord.GetRecord();
            iItem = NONE;
        }
        else {
            iRec = pView->m_gridRecord.GetRecord(iRow);
            iItem = pView->m_gridRecord.GetItem(iRow);
        }
        iVSet  = NONE;
        iValue = NONE;
    }
    else if (pView->m_iGrid == DictionaryGrid::Item) {
        if (pView->m_gridItem.IsEditing()) {
            return 0;
        }
        iLevel = pView->m_gridItem.GetLevel();
        iRec = pView->m_gridItem.GetRecord();
        iItem = pView->m_gridItem.GetItem();
        iRow = pView->m_gridItem.GetCurrentRow();
        if (iRow >= 0 && pView->m_gridItem.HasVSets() ) {      // BMD 30 Jul 2003; second condition 20120926
            iVSet = pView->m_gridItem.GetVSet(iRow);
            iValue = pView->m_gridItem.GetValue(iRow);
        }
        else {
            iVSet = NONE;
            iValue = NONE;
        }
    }

    // Get find specs
    bool bNext = DictFind::dlg.IsNext();
    bool bCaseSensitive = (DictFind::dlg.IsCaseSensitive() ? true:false);
    CIMSAString csFindText = DictFind::dlg.GetFindText();
    if (!bCaseSensitive) {
        csFindText.MakeUpper();
    }

    // Find match
    CDataDict* pDict = pDoc->GetDict();
    bool bFound = pDict->Find(bNext, bCaseSensitive, csFindText, iLevel, iRec, iItem, iVSet, iValue);
    if (!bFound) {
        if (bNext) {
            iLevel = iRec = iItem = iVSet = iValue = NONE;
        }
        else {
            iLevel = iRec = iItem = iVSet = iValue = INT_MAX;
        }
        bFound = pDict->Find(bNext, bCaseSensitive, csFindText, iLevel, iRec, iItem, iVSet, iValue);
    }
    if (!bFound) {
        AfxMessageBox(_T("Not found"));
        return 0;
    }

    // Reposition grids
    pDoc->SetLevel(iLevel);
    pDoc->SetRec(iRec);
    pDoc->SetItem(iItem);
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    if (iRec == NONE)  {
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel);
        pView->m_iGrid = DictionaryGrid::Dictionary;
        pView->m_gridDict.SetRedraw(FALSE);
        pView->m_gridDict.Update(pDict);
        pView->ResizeGrid();
        pView->m_gridDict.ClearSelections();
        if (iLevel == NONE) {
            pView->m_gridDict.GotoCell(1,0);
        }
        else {
            pView->m_gridDict.GotoCell(1, iLevel + CDictGrid::GetFirstLevelRow());
        }
        pView->m_gridDict.SetRedraw(TRUE);
        pView->m_gridDict.InvalidateRect(NULL);
        pView->m_gridDict.SetFocus();
    }
    else if (iItem == NONE)  {
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel, iRec);
        pView->m_iGrid = DictionaryGrid::Level;
        pView->m_gridLevel.SetRedraw(FALSE);
        pView->m_gridLevel.Update(pDoc->GetDict(), iLevel);
        pView->ResizeGrid();
        pView->m_gridLevel.ClearSelections();
        pView->m_gridLevel.GotoCell(1, pView->m_gridLevel.GetFirstRow() + iRec);
        pView->m_gridLevel.SetRedraw(TRUE);
        pView->m_gridLevel.InvalidateRect(NULL);
        pView->m_gridLevel.SetFocus();
    }
    else if (iVSet == NONE)  {
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel, iRec);
        pView->m_iGrid = DictionaryGrid::Record;
        pView->m_gridRecord.SetRedraw(FALSE);
        pView->m_gridRecord.Update(pDoc->GetDict(), iLevel, iRec);
        pView->ResizeGrid();
        pView->m_gridRecord.ClearSelections();
        if (iItem == NONE) {
            ASSERT(FALSE);
        }
        else {
            pView->m_gridRecord.GotoCell(1, pView->m_gridRecord.GetRow(iItem));
        }
        pView->m_gridRecord.SetRedraw(TRUE);
        pView->m_gridRecord.InvalidateRect(NULL);
        pView->m_gridRecord.SetFocus();
    }
    else {
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel, iRec, iItem);
        pView->m_iGrid = DictionaryGrid::Item;
        pView->m_gridItem.Update(pDoc->GetDict(), iLevel, iRec, iItem, iVSet);
        pView->ResizeGrid();
        pView->m_gridItem.ClearSelections();
        pView->m_gridItem.GotoCell(1, pView->m_gridItem.GetRow(iVSet, iValue));
        pView->m_gridItem.InvalidateRect(NULL);
        pView->m_gridItem.SetFocus();
    }
    return 0;
}


void CDictChildWnd::OnFilePageSetup() {

    CIMSAPageSetupDlg dlg;
    dlg.DoModal();
}


void CDictChildWnd::OnViewAliases()
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
    const CDataDict* pDict = pDoc->GetDict();

    std::vector<std::tuple<CString, CString>> aliases;
    int longest_name = 0;

    DictionaryIterator::ForeachDictNamedBase(*pDict,
        [&](const DictNamedBase& dict_element)
        {
            for( const CString& alias : dict_element.GetAliases() )
            {
                aliases.emplace_back(alias, dict_element.GetName());
                longest_name = std::max(longest_name, alias.GetLength());
            }
        });

    if( aliases.empty() )
    {
        AfxMessageBox(_T("There are no defined aliases."));
    }

    else
    {
        // show the aliases in sorted order
        CString alias_report;

        std::sort(aliases.begin(), aliases.end());

        for( const auto& [name, alias] : aliases )
            alias_report.AppendFormat(_T("%-*s  ->  %s\r\n"), longest_name, (LPCTSTR)name, (LPCTSTR)alias);

        TextReportDlg text_report_dialog(_T("Dictionary Aliases"), alias_report);
        text_report_dialog.UseFixedWidthFont();
        text_report_dialog.DoModal();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        Dictionary Analysis Methods
//
/////////////////////////////////////////////////////////////////////////////

void CDictChildWnd::RunDictAnalysis(std::function<void(const CDictItem&)>& analysis_function,
    std::function<bool(CString&, CString&)>& summary_results_function)
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());

    // call the analysis function for each item
    DictionaryIterator::Foreach<CDictItem>(*pDoc->GetDict(),
        [&](const CDictItem& dict_item)
        {
            analysis_function(dict_item);
        });

    // get and display the report text
    CString heading;
    CString contents;

    if( summary_results_function(heading, contents) )
    {
        TextReportDlg text_report_dialog(heading, contents);
        text_report_dialog.DoModal();
    }

    else
    {
        AfxMessageBox(_T("No items exist that meet the selected criteria."));
    }
}

void CDictChildWnd::DisplayActiveMode() {
    if (m_bQuestionnaireView) {
        OnViewQuestionnaire();
    }
    else {
        CRect rect;
        AfxGetMainWnd()->GetClientRect(&rect);
        int height = rect.Height();
        m_wndSplitter.GetPane(0, 0)->ShowWindow(SW_SHOW);
        m_wndSplitter.GetPane(1, 0)->ShowWindow(SW_SHOW);
        m_wndSplitter.GetPane(2, 0)->ShowWindow(SW_HIDE);

        int gridViewHeight = 2 * height / 3;
        int layoutHeight = height / 3;
        if (m_iGridViewPaneSize > 0) {
            gridViewHeight = m_iGridViewPaneSize;
            layoutHeight = height - m_iGridViewPaneSize;
        }
        m_wndSplitter.SetSplitterBarSizes();
        m_wndSplitter.SetRowInfo(0, gridViewHeight, 0);
        m_wndSplitter.SetRowInfo(1, layoutHeight, 0);
        m_wndSplitter.SetRowInfo(2, 0, 0);
        m_wndSplitter.RecalcLayout();
    }
}
void CDictChildWnd::RunDictAnalysisNoValueSets(bool numerics_only)
{
    int number_items_without_value_sets = 0;
    CString items_without_value_sets;

    std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item) -> void
        {
            if( !DictionaryRules::CanHaveValueSet(dict_item) || ( numerics_only && dict_item.GetContentType() != ContentType::Numeric ) )
                return;

            if( !dict_item.HasValueSets() )
            {
                ++number_items_without_value_sets;
                items_without_value_sets.AppendFormat(_T("%s\r\n"), (LPCTSTR)dict_item.GetName());
            }
        };

    std::function<bool(CString&, CString&)> summary_results_function =
        [&](CString& heading, CString& contents) -> bool
        {
            if( number_items_without_value_sets == 0 )
                return false;

            heading.Format(_T("There are %d numeric%s item%s without value sets:"), number_items_without_value_sets,
                           numerics_only ? _T("") : _T("/alpha"), PluralizeWord(number_items_without_value_sets));

            contents = items_without_value_sets;

            return true;
        };

    RunDictAnalysis(analysis_function, summary_results_function);
}

void CDictChildWnd::OnDictAnalysisItemsNoValueSets()
{
    RunDictAnalysisNoValueSets(false);
}

void CDictChildWnd::OnDictAnalysisNumericItemsNoValueSets()
{
    RunDictAnalysisNoValueSets(true);
}


namespace
{
    inline void ThrowIfDiscreteValueFallsWithinRange(double discrete_value, const ValueSetResponse& range)
    {
        if( discrete_value >= range.GetMinimumValue() && discrete_value <= range.GetMaximumValue() )
            throw std::exception();
    }

    bool DoesValueSetHaveOverlappingRanges(const CDictItem& dict_item, const DictValueSet& dict_value_set)
    {
        std::set<double> discretes;
        std::vector<std::shared_ptr<const ValueSetResponse>> ranges;
        bool has_overlapping_ranges = false;

        try
        {
            for( const auto& dict_value : dict_value_set.GetValues() )
            {
                for( const auto& dict_value_pair : dict_value.GetValuePairs() )
                {
                    // use ValueSetResponse to easily parse each pair
                    auto value_set_response = std::make_shared<const ValueSetResponse>(dict_item, dict_value, dict_value_pair);

                    if( value_set_response->IsDiscrete() )
                    {
                        double discrete_value = value_set_response->GetMinimumValue();

                        if( discretes.find(discrete_value) != discretes.end() )
                            throw std::exception();

                        discretes.insert(discrete_value);
                    }

                    else
                    {
                        ranges.emplace_back(value_set_response);
                    }
                }
            }

            // in the above loop, any duplicate discrete values will be detected; now check for range overlaps

            // first check if the discretes are in any of the ranges
            for( double discrete_value : discretes )
            {
                for( const auto& range : ranges )
                    ThrowIfDiscreteValueFallsWithinRange(discrete_value, *range);
            }

            // now check if any of the ranges overlap any of the other ranges
            for( const auto& range1 : ranges )
            {
                for( const auto& range2 : ranges )
                {
                    if( range1 != range2 )
                    {
                        ThrowIfDiscreteValueFallsWithinRange(range1->GetMinimumValue(), *range2);
                        ThrowIfDiscreteValueFallsWithinRange(range1->GetMaximumValue(), *range2);
                    }
                }
            }
        }

        catch(...)
        {
            has_overlapping_ranges = true;
        }

        return has_overlapping_ranges;
    }
}

void CDictChildWnd::OnDictAnalysisNumericItemsOverlappingValueSets()
{
    int number_overlapping_value_sets = 0;
    CString overlapping_value_sets;

    std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item) -> void
        {
            if( dict_item.GetContentType() != ContentType::Numeric )
                return;

            for( const auto& dict_value_set : dict_item.GetValueSets() )
            {
                if( DoesValueSetHaveOverlappingRanges(dict_item, dict_value_set) )
                {
                    ++number_overlapping_value_sets;
                    overlapping_value_sets.AppendFormat(_T("%s (%s)\r\n"), (LPCTSTR)dict_item.GetName(), (LPCTSTR)dict_value_set.GetName());
                }
            }
        };

    std::function<bool(CString&, CString&)> summary_results_function =
        [&](CString& heading, CString& contents) -> bool
        {
            if( number_overlapping_value_sets == 0 )
                return false;

            heading.Format(_T("There are %d numeric value set%s with overlapping ranges:"),
                           number_overlapping_value_sets, PluralizeWord(number_overlapping_value_sets));

            contents = overlapping_value_sets;

            return true;
        };

    RunDictAnalysis(analysis_function, summary_results_function);
}


void CDictChildWnd::OnDictAnalysisNumericMismatchedZeroFillDecChar()
{
    int number_mismatched_items = 0;
    CString mismatched_items;

    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
    const CDataDict* pDict = pDoc->GetDict();
    bool default_zero_fill = pDict->IsZeroFill();
    bool default_dec_char = pDict->IsDecChar();

    std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item) -> void
        {
            if( dict_item.GetContentType() != ContentType::Numeric )
                return;

            bool zero_fill_mismatch = ( dict_item.GetZeroFill() != default_zero_fill );
            bool dec_char_mismatch = ( dict_item.GetDecimal() > 0 && dict_item.GetDecChar() != default_dec_char );

            if( zero_fill_mismatch || dec_char_mismatch )
            {
                ++number_mismatched_items;
                mismatched_items.AppendFormat(_T("%s (%s%s%s)\r\n"),
                    (LPCTSTR)dict_item.GetName(),
                    zero_fill_mismatch ? _T("zero fill") : _T(""),
                    ( zero_fill_mismatch && dec_char_mismatch ) ? _T(" / ") : _T(""),
                    dec_char_mismatch ? _T("decimal character") : _T(""));
            }
        };

    std::function<bool(CString&, CString&)> summary_results_function =
        [&](CString& heading, CString& contents) -> bool
        {
            if( number_mismatched_items == 0 )
                return false;

            heading.Format(_T("There are %d item%s with mismatched options:"),
                number_mismatched_items, PluralizeWord(number_mismatched_items));

            contents = mismatched_items;

            return true;
        };

    RunDictAnalysis(analysis_function, summary_results_function);
}


void CDictChildWnd::OnViewProperties()
{
    if (m_bViewPropertiesPanel && m_dictDlgBar.GetSafeHwnd() != NULL) {
        m_bViewPropertiesPanel = FALSE;
        ShowControlBar(&m_dictDlgBar, FALSE, FALSE);
    }
    else {
        m_bViewPropertiesPanel = TRUE;
        if (m_dictDlgBar.GetSafeHwnd() == NULL) {
            // create the reference window
            if (!m_dictDlgBar.CreateAndDock(this))
                return;
        }
        ShowControlBar(&m_dictDlgBar, TRUE, FALSE);
        CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
        pDoc->GetGrid()->OnRowChange(0, 0);//force property panel update
    }
    m_wndSplitter.RecalcLayout();
}


void CDictChildWnd::OnUpdateViewProperties(CCmdUI* pCmdUI)
{
    if (m_bQuestionnaireView) {
        pCmdUI->Enable(FALSE);
    }
    else {
        pCmdUI->Enable(TRUE);
        pCmdUI->SetCheck(m_bViewPropertiesPanel);
    }
}


void CDictChildWnd::OnViewQuestionnaire()
{
    m_bQuestionnaireView = TRUE;
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
    if (!pDoc)
        return;
    if (!m_pQuestionnaireView) {
        CWnd* pWnd = m_wndSplitter.GetPane(2, 0);
        int iID = m_wndSplitter.IdFromRowCol(2, 0);
        pWnd->SetDlgCtrlID(1116);
        CRect rect(0, 0, 100, 100);
        pWnd->GetClientRect(&rect);

        DWORD dwStyle = AFX_WS_DEFAULT_VIEW;
        dwStyle &= ~WS_BORDER;

        m_pQuestionnaireView = new QuestionnaireView(*pDoc);
        CCreateContext context = CCreateContext();
        context.m_pCurrentDoc = pDoc;
        if (!m_pQuestionnaireView->Create(NULL, _T(""), dwStyle, rect, &m_wndSplitter, iID, &context)) {
            TRACE0("Failed to create questionnaire view\n");
            ASSERT(false);
            return;
        }
        m_pQuestionnaireView->SendMessage(WM_INITIALUPDATE);
    }
    else{
        int iTempID = m_pQuestionnaireView->GetDlgCtrlID();
        int iID = m_wndSplitter.IdFromRowCol(2, 0);
        CWnd* pWnd = m_wndSplitter.GetPane(2, 0);
        //Set the controlId of the Questionnaire view to this pane
        m_pQuestionnaireView->SetDlgCtrlID(iID);
        m_pQuestionnaireView->ShowWindow(SW_SHOW);
        pWnd->SetDlgCtrlID(iTempID);
        m_pQuestionnaireView->RefreshContent(false);
    }
    if (m_bViewPropertiesPanel && m_dictDlgBar.GetSafeHwnd() != NULL) {
        m_bViewPropertiesPanel = FALSE;
        ShowControlBar(&m_dictDlgBar, FALSE, FALSE);
    }
    m_wndSplitter.SetActivePane(2, 0);
    CRect rect;
    AfxGetMainWnd()->GetClientRect(&rect);
    m_wndSplitter.SetSplitterBarSizes();
    m_wndSplitter.SetRowInfo(0, 0, 0);
    m_wndSplitter.SetRowInfo(1, 0, 0);
    m_wndSplitter.SetRowInfo(2, rect.Height(), 10);
    m_wndSplitter.GetPane(0, 0)->ShowWindow(SW_HIDE);
    m_wndSplitter.GetPane(1, 0)->ShowWindow(SW_HIDE);

    m_wndSplitter.RecalcLayout();
    CView* pView = GetActiveView();
    pView->SetFocus();
    //setting language here crashes webview
   //m_pQuestionnaireView->ChangeLanguage(pDoc->GetDict()->GetCurrentLanguage().GetName());
    return;
}


void CDictChildWnd::OnUpdateViewQuestionnaire(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
    if (m_bQuestionnaireView) {
        pCmdUI->SetCheck(TRUE);
    }
    else {
        pCmdUI->SetCheck(FALSE);
    }
}


void CDictChildWnd::OnUpdateEditLanguages(CCmdUI* pCmdUI)
{
    m_bQuestionnaireView ? pCmdUI->Enable(FALSE) : pCmdUI->Enable(TRUE);
}


void CDictChildWnd::OnUpdateEditRelation(CCmdUI* pCmdUI)
{
    m_bQuestionnaireView ? pCmdUI->Enable(FALSE) : pCmdUI->Enable(TRUE);
}


void CDictChildWnd::OnUpdateEditSecurityOptions(CCmdUI* pCmdUI)
{
    m_bQuestionnaireView ? pCmdUI->Enable(FALSE) : pCmdUI->Enable(TRUE);
}


void CDictChildWnd::OnUpdateDictionaryMacros(CCmdUI* pCmdUI)
{
    m_bQuestionnaireView ? pCmdUI->Enable(FALSE) : pCmdUI->Enable(TRUE);
}


void CDictChildWnd::OnUpdateOptionsEnableBinaryItems(CCmdUI* pCmdUI)
{
    if (m_bQuestionnaireView) {
        pCmdUI->Enable(FALSE);
    }
    else {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
        pCmdUI->SetCheck(pDoc->GetDict()->EnableBinaryItems());
        pCmdUI->Enable(!pDoc->IsEditing());
    }
}


void CDictChildWnd::OnUpdateOptionsAbsRel(CCmdUI* pCmdUI)
{
    if (m_bQuestionnaireView) {
        pCmdUI->Enable(FALSE);
    }
    else {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
        if (pDoc && pDoc->GetDict()) {
            pCmdUI->SetCheck(pDoc->GetDict()->IsPosRelative());
            pCmdUI->Enable(!pDoc->IsEditing());
        }
    }
}

void CDictChildWnd::OnUpdateOptionsZeroFill(CCmdUI* pCmdUI)
{
    if (m_bQuestionnaireView) {
        pCmdUI->Enable(FALSE);
    }
    else {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
        if (pDoc && pDoc->GetDict()) {
            pCmdUI->SetCheck(pDoc->GetDict()->IsZeroFill());
            pCmdUI->Enable(!pDoc->IsEditing());
        }
    }
}


void CDictChildWnd::OnUpdateOptionsDecChar(CCmdUI* pCmdUI)
{
    if (m_bQuestionnaireView) {
        pCmdUI->Enable(FALSE);
    }
    else {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
        if (pDoc && pDoc->GetDict()) {
            pCmdUI->SetCheck(pDoc->GetDict()->IsDecChar());
            pCmdUI->Enable(!pDoc->IsEditing());
        }
    }
}


void CDictChildWnd::OnViewDictionary()
{
    if (!m_bQuestionnaireView)
        return;
    m_bQuestionnaireView = FALSE;
    CRect rect;
    AfxGetMainWnd()->GetClientRect(&rect);
    int height = rect.Height();
    m_wndSplitter.GetPane(0, 0)->ShowWindow(SW_SHOW);
    m_wndSplitter.GetPane(1, 0)->ShowWindow(SW_SHOW);
    m_wndSplitter.GetPane(2, 0)->ShowWindow(SW_HIDE);
    if (!m_bLayout) {
        m_wndSplitter.SetRowInfo(0, height, 0);
        m_wndSplitter.SetRowInfo(1, 0, 0);
        m_wndSplitter.SetRowInfo(2, 0, 0);
    }
    else {
        m_wndSplitter.SetRowInfo(0, 2 * height / 3, 0);
        m_wndSplitter.SetRowInfo(1, height / 3, 0);
        m_wndSplitter.SetRowInfo(2, 0, 0);
    }
    m_wndSplitter.SetSplitterBarSizes();
    m_wndSplitter.RecalcLayout();
}


void CDictChildWnd::OnUpdateViewDictionary(CCmdUI* pCmdUI)
{
    m_bQuestionnaireView ? pCmdUI->SetCheck(FALSE) : pCmdUI->SetCheck(TRUE);
}
