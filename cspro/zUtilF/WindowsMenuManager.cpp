#include "StdAfx.h"
#include "WindowsMenuManager.h"
#include "WindowsDocsDlg.h"


CDocument* WindowsMenuManager::GetActiveDocument(CMDIFrameWnd& frame_wnd)
{
    CMDIChildWnd* active_wnd = frame_wnd.MDIGetActive();
    return ( active_wnd != nullptr ) ? active_wnd->GetActiveDocument() : nullptr;
}


std::vector<CDocument*> WindowsMenuManager::GetDocuments(CMDIFrameWnd& frame_wnd, bool add_active_document_first, unsigned max_documents_to_add)
{
    std::vector<CDocument*> documents;

    CDocument* active_doc = add_active_document_first ? GetActiveDocument(frame_wnd) : nullptr;

    if( active_doc != nullptr )
        documents.emplace_back(active_doc);

    POSITION template_pos = AfxGetApp()->GetFirstDocTemplatePosition();

    while( template_pos != nullptr )
    {
        CDocTemplate* doc_template = AfxGetApp()->GetNextDocTemplate(template_pos);
        POSITION doc_pos = doc_template->GetFirstDocPosition();

        while( doc_pos != nullptr )
        {
            CDocument* doc = doc_template->GetNextDoc(doc_pos);

            if( doc != active_doc )
            {
                documents.emplace_back(doc);

                if( documents.size() >= max_documents_to_add )
                    return documents;
            }
        }
    }

    return documents;
}


void WindowsMenuManager::Build(CMDIFrameWnd& frame_wnd, CMenu* pPopupMenu)
{
    const std::vector<CDocument*> documents = GetDocuments(frame_wnd, true, MaxDocumentCount);

    if( documents.empty() )
        return;

    pPopupMenu->AppendMenu(MF_SEPARATOR);

    for( unsigned doc_index = 0; doc_index < documents.size(); ++doc_index )
    {
        const CDocument* doc = documents[doc_index];
        const unsigned flags = MF_STRING | ( ( doc_index == 0 ) ? MF_CHECKED : 0 );

        CString menu_text = doc->GetTitle();

        ASSERT(doc_index <= 9);
        menu_text.Insert(0, ( doc_index == 9 ) ? _T("1&0 ") : FormatText(_T("&%d "), doc_index + 1));

        pPopupMenu->AppendMenu(flags, DocumentFirstId + doc_index, menu_text);
    };

    pPopupMenu->AppendMenu(MF_STRING, ID_WINDOWS_WINDOWS, _T("&Windows..."));
}


void WindowsMenuManager::ActivateDocument(CMDIFrameWnd& frame_wnd, UINT nID)
{
    unsigned doc_index = nID - DocumentFirstId;

    // the first index is the active document, so no need to change documents
    if( doc_index == 0 )
        return;

    const std::vector<CDocument*> documents = GetDocuments(frame_wnd, true, doc_index + 1);
    ASSERT(documents.size() == ( doc_index + 1));

    CDocument* doc_to_activate = documents[doc_index];

    POSITION pos = doc_to_activate->GetFirstViewPosition();

    if( pos != nullptr )
    {
        CView* view = doc_to_activate->GetNextView(pos);
        view->GetParentFrame()->ActivateFrame();
    }
}


void WindowsMenuManager::ShowWindowsDocDlg(CMDIFrameWnd& frame_wnd)
{
    WindowsDocsDlg dlg(GetDocuments(frame_wnd, false, std::numeric_limits<unsigned>::max()), GetActiveDocument(frame_wnd));
    dlg.DoModal();
}
