#include "StdAfx.h"
#include "WindowsDocsDlg.h"


BEGIN_MESSAGE_MAP(WindowsDocsDlg, CDialog)
    ON_MESSAGE(UWM::UtilF::UpdateDialogUI, OnUpdateDialogUI)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_DOC_LIST, OnDocListItemChange)
    ON_BN_CLICKED(IDC_ACTIVATE, OnOK)
    ON_BN_CLICKED(IDC_SAVE, OnSave)
    ON_BN_CLICKED(IDC_CLOSE_WINDOWS, OnCloseWindows)
END_MESSAGE_MAP()


WindowsDocsDlg::WindowsDocsDlg(std::vector<CDocument*> docs, const CDocument* active_doc, CWnd* pParent/*= nullptr*/)
    :   CDialog(IDD_WINDOWS_DOCS, pParent),
        m_docs(std::move(docs)),
        m_activeDoc(active_doc),
        m_activateButton(nullptr),
        m_saveButton(nullptr),
        m_closeWindowsButton(nullptr)
{
    ASSERT(!m_docs.empty() && m_activeDoc != nullptr);
}


void WindowsDocsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    
    DDX_Control(pDX, IDC_DOC_LIST, m_docList);
}


BOOL WindowsDocsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_docList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_docList.SetHeadings(_T("Name,200;Path,350"));
    m_docList.LoadColumnInfo();

    // fill in the list of documents
    for( CDocument* doc : m_docs )
        m_docList.AddItem(doc->GetTitle().GetString(), doc->GetPathName().GetString());

    // preselect the active documnet
    m_docList.SetItemState(GetDocIndex(m_activeDoc), LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    m_docList.SetFocus();

    // get handles to the buttons and then update the UI
    m_activateButton = static_cast<CButton*>(GetDlgItem(IDC_ACTIVATE));
    m_saveButton = static_cast<CButton*>(GetDlgItem(IDC_SAVE));
    m_closeWindowsButton = static_cast<CButton*>(GetDlgItem(IDC_CLOSE_WINDOWS));

    PostMessage(UWM::UtilF::UpdateDialogUI);

    return FALSE;
}


std::vector<CDocument*> WindowsDocsDlg::GetSelectedDocs() const
{
    std::vector<CDocument*> selected_docs;

    POSITION pos = m_docList.GetFirstSelectedItemPosition();

    while( pos != nullptr )
    {
        int index = m_docList.GetNextSelectedItem(pos);
        selected_docs.emplace_back(m_docs[index]);
    }

    return selected_docs;
}


size_t WindowsDocsDlg::GetDocIndex(const CDocument* doc) const
{
    ptrdiff_t index = std::distance(m_docs.cbegin(),
                                    std::find(m_docs.cbegin(), m_docs.cend(), doc));
    ASSERT(index >= 0 && index < static_cast<ptrdiff_t>(m_docs.size()));

    return static_cast<size_t>(index);
}


CFrameWnd* WindowsDocsDlg::GetDocParentFrame(const CDocument* doc) const
{
    POSITION pos = ( doc != nullptr ) ? doc->GetFirstViewPosition() : nullptr;

    if( pos != nullptr )
    {
        CView* view = doc->GetNextView(pos);
        return view->GetParentFrame();
    }

    return nullptr;
}


LRESULT WindowsDocsDlg::OnUpdateDialogUI(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    std::vector<CDocument*> selected_docs = GetSelectedDocs();
    bool document_was_modified = std::any_of(selected_docs.cbegin(), selected_docs.cend(),
                                             [&](CDocument* doc) { return doc->IsModified(); });

    m_activateButton->EnableWindow(selected_docs.size() == 1);
    m_saveButton->EnableWindow(document_was_modified);
    m_closeWindowsButton->EnableWindow(!selected_docs.empty());

    return 1;
}


void WindowsDocsDlg::OnDocListItemChange(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    PostMessage(UWM::UtilF::UpdateDialogUI);
    *pResult = 0;
}


void WindowsDocsDlg::OnOK()
{
    std::vector<CDocument*> selected_docs = GetSelectedDocs();

    if( !selected_docs.empty() )
    {
        CFrameWnd* parent_frame = GetDocParentFrame(selected_docs.front());

        if( parent_frame != nullptr )
            parent_frame->ActivateFrame();
    }

    __super::OnOK();
}


void WindowsDocsDlg::OnSave()
{
    for( CDocument* doc : GetSelectedDocs() )
    {
        if( doc->IsModified() )
        {
            // update the name because the modified flag in the title should now be gone
            if( doc->DoFileSave() )
                m_docList.SetItemText(GetDocIndex(doc), 0, doc->GetTitle());
        }
    }
}


void WindowsDocsDlg::OnCloseWindows()
{
    std::set<const CDocument*> closed_documents;

    for( CDocument* doc : GetSelectedDocs() )
    {
        if( doc->SaveModified() )
        {
            doc->OnCloseDocument();
            closed_documents.insert(doc);
        }
    }

    if( closed_documents.empty() )
        return;

    // it is possible that an overriden OnCloseDocument prevented a document from closing, so ensure
    // that all documents were actually closed
    POSITION template_pos = AfxGetApp()->GetFirstDocTemplatePosition();

    while( template_pos != nullptr )
    {
        CDocTemplate* doc_template = AfxGetApp()->GetNextDocTemplate(template_pos);
        POSITION doc_pos = doc_template->GetFirstDocPosition();

        while( doc_pos != nullptr )
            closed_documents.erase(doc_template->GetNextDoc(doc_pos));
    }

    // remove the entries from the list
    for( const CDocument* closed_document : closed_documents )
    {
        size_t doc_index = GetDocIndex(closed_document);
        m_docs.erase(m_docs.begin() + doc_index);
        m_docList.DeleteItem(doc_index);
    }
}
