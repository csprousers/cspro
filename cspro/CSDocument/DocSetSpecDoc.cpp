#include "StdAfx.h"
#include "DocSetSpecDoc.h"


IMPLEMENT_DYNCREATE(DocSetSpecDoc, TextEditDoc)


void DocSetSpecDoc::CreateNewDocument()
{
    const std::string filter = FormatText("CSPro Document Sets (*.%s)|*.%s|All Files (*.*)|*.*||",
                                          FileExtensions::CSDocumentSet, FileExtensions::CSDocumentSet);

    SaveFileDlg save_file_dlg(0, FileExtensions::CSDocumentSet, nullptr, filter, nullptr);
    save_file_dlg.SetTitle(L"Create Document Set");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    try
    {
        const std::string& file_path = save_file_dlg.GetFilePath();

        DocSetSpec::WriteNewDocumentSetShell(file_path);

        AfxGetApp()->OpenDocumentFile(TC::ToWide(file_path).c_str());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


BOOL DocSetSpecDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    // the primary document type is .csdoc so we should only be here if opening a .csdocset
    ASSERT(SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(lpszPathName), FileExtensions::CSDocumentSet));

    if( !__super::OnOpenDocument(lpszPathName) )
        return FALSE;

    m_docSetSpec = assert_cast<CMainFrame*>(AfxGetMainWnd())->FindSharedDocSetSpec(TC::ToUtf8(lpszPathName), true);
    ASSERT(m_docSetSpec != nullptr);

    return TRUE;
}
