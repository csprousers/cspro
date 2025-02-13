#include "StdAfx.h"
#include "Csdfview.h"
#include "Csdfdoc.h"


IMPLEMENT_DYNCREATE(CCSDiffView, CFormView)

BEGIN_MESSAGE_MAP(CCSDiffView, CFormView)
    ON_WM_SIZE()
    ON_COMMAND(ID_TOGGLE, OnToggle)
    ON_UPDATE_COMMAND_UI(ID_TOGGLE, OnUpdateToggle)
END_MESSAGE_MAP()


CCSDiffView::CCSDiffView()
    :   CFormView(CCSDiffView::IDD)
{
    m_dictionaryTreeCtrl.show_binary_items = true;
}


void CCSDiffView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DATADICT_TREE, m_dictionaryTreeCtrl);
}


std::wstring CCSDiffView::CreateWindowTitle(const std::wstring& spec_filename, const CDataDict* dictionary)
{
    return FormatTextCS2WS(_T("CSPro Compare Data - [Spec File = %s / Dictionary = %s]"),
                           !spec_filename.empty() ? PortableFunctions::PathGetFilename(spec_filename) : _T("Untitled"),
                           ( dictionary != nullptr ) ? PortableFunctions::PathGetFilename(dictionary->GetFullFileName()) : _T(""));
}


void CCSDiffView::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();

    CCSDiffDoc* pDoc = static_cast<CCSDiffDoc*>(GetDocument());
    ASSERT(pDoc != nullptr);

    m_dictionaryTreeCtrl.SetParent(this);

    // make sure that parent frame is large enough to completely show the form view
    GetParentFrame()->RecalcLayout();
    UpdateWindow();

    const std::shared_ptr<const CDataDict> dictionary = pDoc->GetDiffSpec().GetSharedDictionary();
    m_dictionaryTreeCtrl.Initialize(dictionary, false, false, false, this);

    AfxGetMainWnd()->SetWindowText(CreateWindowTitle(CS2WS(pDoc->GetPff().GetAppFName()), dictionary.get()).c_str());
}


void CCSDiffView::OnSize(const UINT nType, const int cx, const int cy)
{
    CFormView::OnSize(nType, cx, cy);

    if( m_dictionaryTreeCtrl.m_hWnd != nullptr )
    {
        CRect rc;
        GetClientRect(&rc);
        m_dictionaryTreeCtrl.MoveWindow(0, 0, rc.Width(), rc.Height(), TRUE);
    }
}


void CCSDiffView::OnToggle()
{
    DiffSpec& diff_spec = GetDocument()->GetDiffSpec();

    diff_spec.SetShowLabels(!diff_spec.GetShowLabels());
    SharedSettings::ToggleViewNamesInTree(!diff_spec.GetShowLabels());

    m_dictionaryTreeCtrl.Invalidate();
}


void CCSDiffView::OnUpdateToggle(CCmdUI* pCmdUI)
{
    const DiffSpec& diff_spec = GetDocument()->GetDiffSpec();
    pCmdUI->SetCheck(!diff_spec.GetShowLabels());
}


bool CCSDiffView::IsDictionaryTreeNodeChecked(const DictionaryTreeNode& dictionary_tree_node) const
{
    ASSERT(dictionary_tree_node.GetDictElementType() == DictElementType::Item);
    const DiffSpec& diff_spec = GetDocument()->GetDiffSpec();

    return diff_spec.IsItemSelected(dictionary_tree_node.GetDictItemOccurrenceInfo());
}


void CCSDiffView::OnDictionaryTreeNodeCheck(const DictionaryTreeNode& dictionary_tree_node, const bool checked)
{
    ASSERT(dictionary_tree_node.GetDictElementType() == DictElementType::Item);
    DiffSpec& diff_spec = GetDocument()->GetDiffSpec();

    GetDocument()->SetModifiedFlag();

    diff_spec.SetItemSelection(dictionary_tree_node.GetDictItemOccurrenceInfo(), checked);
}
