#pragma once

#include <zUtilF/zUtilF.h>
#include <zUtilF/resource_shared.h>


class CLASS_DECL_ZUTILF WindowsMenuManager
{
public:
    static constexpr unsigned MaxDocumentCount = 10;
    static constexpr unsigned DocumentFirstId  = AFX_IDM_FIRST_MDICHILD;
    static constexpr unsigned DocumentLastId   = DocumentFirstId + MaxDocumentCount - 1;

    // adds open documents to the Windows menu with the active document as the first entry
    // and adds the "Windows" option to display WindowsDocsDlg
    static void Build(CMDIFrameWnd& frame_wnd, CMenu* pPopupMenu);

    // activates the document selected from the Windows menu
    static void ActivateDocument(CMDIFrameWnd& frame_wnd, UINT nID);

    // shows the WindowsDocsDlg
    static void ShowWindowsDocDlg(CMDIFrameWnd& frame_wnd);

private:
    static CDocument* GetActiveDocument(CMDIFrameWnd& frame_wnd);
    static std::vector<CDocument*> GetDocuments(CMDIFrameWnd& frame_wnd, bool add_active_document_first, unsigned max_documents_to_add);
};
